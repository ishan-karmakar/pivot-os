/// Implementation of the buddy allocator (https://en.wikipedia.org/wiki/Buddy_memory_allocation)
/// Metadata is stored external to the arena, so size must be known at initialization
/// Uses an efficient bitmap under the hood to save space
const std = @import("std");
const Tuple = std.meta.Tuple;
const kernel = @import("kernel");
const log = std.log.scoped(.vmm);
const math = std.math;
const Allocator = std.mem.Allocator;
const mem = kernel.lib.mem;
const Mutex = kernel.lib.Mutex;
const Self = @This();

const Block = struct {
    depth: usize,
    col: usize,
    bsize: usize,
};

bitmap: []u8,
start: usize,
internal_blocks: usize,
max_bsize: usize,
mapper: *mem.Mapper,
flags: u64,

pub fn create(start: usize, size: usize, flags: u64, mapper: *mem.Mapper) Self {
    const sizes = calc_split(size);
    // TODO: is meta size guaranteed to be page size multiple?
    const pages = math.divCeil(usize, sizes[2], 0x1000) catch unreachable;
    for (0..pages) |i| {
        const frm = mem.pmm.frame();
        mapper.map(frm, start + i * 0x1000, flags | 0b10 | (1 << 63));
    }
    const bitmap = @as([*]u8, @ptrFromInt(start))[0..(pages * 0x1000)];
    @memset(bitmap, 0);
    var vmm = Self{
        .bitmap = bitmap,
        .start = start + pages * 0x1000,
        .flags = flags,
        .internal_blocks = sizes[3],
        .mapper = mapper,
        .max_bsize = sizes[0],
    };
    vmm.rsv_extra(Block{
        .depth = 0,
        .col = 0,
        .bsize = vmm.max_bsize,
    }, sizes[1]);
    return vmm;
}

pub fn allocator(self: *Self) Allocator {
    return .{
        .ptr = self,
        .vtable = &.{
            .alloc = alloc,
            .resize = resize,
            .free = free,
        },
    };
}

fn rsv_extra(self: *Self, block: Block, size: usize) void {
    const start = block.col * block.bsize;
    const end = start + block.bsize;
    if (start >= size) {
        self.set_status(block, 0b1);
    } else if (end > size) {
        var child = Block{
            .depth = block.depth + 1,
            .col = block.col * 2,
            .bsize = block.bsize / 2,
        };
        self.rsv_extra(child, size);
        child.col += 1;
        self.rsv_extra(child, size);
    }
}

pub fn alloc(ctx: *anyopaque, len: usize, _: u8, _: usize) ?[*]u8 {
    // TODO: Maybe implement pointer alignment, but right now no need for it and will unnecessarily complicate code
    const self: *Self = @ptrCast(@alignCast(ctx));
    const bsize = @max(0x1000, math.ceilPowerOfTwoAssert(usize, len));
    if (bsize > self.max_bsize) {
        return null;
    }
    // self.mutex.lock();
    var block = self.alloc_traverse(Block{
        .depth = 0,
        .col = 0,
        .bsize = self.max_bsize,
    }, bsize) orelse return null;
    block = if (block.bsize == bsize) block else self.split_block(block, bsize);
    self.set_status(block, 1);
    // self.mutex.unlock();
    const addr = self.block_addr(block);
    for (0..math.divCeil(usize, len, 0x1000) catch unreachable) |i| {
        const frm = mem.pmm.frame();
        self.mapper.map(frm, addr + i * 0x1000, self.flags);
    }
    return @ptrFromInt(addr);
}

pub fn free(ctx: *anyopaque, buf: []u8, _: u8, _: usize) void {
    const self: *Self = @ptrCast(@alignCast(ctx));
    const bsize = @max(0x1000, math.ceilPowerOfTwoAssert(usize, buf.len));
    if (bsize > self.max_bsize) return;

    const block = Block{
        .depth = math.log2(self.max_bsize / bsize),
        .col = (@intFromPtr(buf.ptr) - self.start) / bsize,
        .bsize = bsize,
    };
    for (0..math.divCeil(usize, buf.len, 0x1000) catch unreachable) |i| {
        mem.pmm.free(mem.kmapper.translate(@intFromPtr(buf.ptr) + i * 0x1000) orelse @panic("Page was not mapped to a physical address"));
    }
    // self.mutex.lock();
    self.merge_buddies(block);
    // self.mutex.unlock();
}

pub fn resize(ctx: *anyopaque, buf: []u8, _: u8, len: usize, _: usize) bool {
    const self: *Self = @ptrCast(@alignCast(ctx));
    const new_bsize = @max(0x1000, math.ceilPowerOfTwoAssert(usize, len));
    const old_bsize = @max(0x1000, math.ceilPowerOfTwoAssert(usize, buf.len));
    if (new_bsize > self.max_bsize) return false;
    const old_pages = math.divCeil(usize, buf.len, 0x1000) catch unreachable;
    const new_pages = math.divCeil(usize, len, 0x1000) catch unreachable;
    const end = @intFromPtr(buf.ptr) + 0x1000 * @min(old_pages, new_pages);
    var new_block: Block = undefined;
    if (old_bsize == new_bsize) {
        return true;
    } else if (new_bsize > old_bsize) {
        // Expanding
        if ((@intFromPtr(buf.ptr) - self.start) % new_bsize != 0) return false;
        // Now is possible, but are the blocks free?
        new_block = Block{
            .depth = math.log2(self.max_bsize / new_bsize),
            .col = (@intFromPtr(buf.ptr) - self.start) / new_bsize,
            .bsize = new_bsize,
        };
        var buddy = new_block;
        buddy.depth += 1;
        buddy.col = buddy.col * 2 + 1;
        // Buddy is not free, cannot resize
        if (self.get_status(buddy) != 0) return false;
        for (0..new_pages - old_pages) |i| {
            mem.kmapper.map(mem.pmm.frame(), end + i * 0x1000, self.flags);
        }
    } else {
        // Shrinking
        const old_block = Block{
            .depth = math.log2(self.max_bsize / old_bsize),
            .col = (@intFromPtr(buf.ptr) - self.start) / old_bsize,
            .bsize = old_bsize,
        };
        new_block = self.split_block(old_block, new_bsize);
        for (0..(old_pages - new_pages)) |i| {
            mem.pmm.free(mem.kmapper.translate(end + i * 0x1000) orelse @panic("Page was not mapped to a physical address"));
        }
    }
    self.set_status(new_block, 0b1);
    return true;
}

fn alloc_traverse(self: Self, node: Block, target_bsize: usize) ?Block {
    const status = self.get_status(node);
    if (node.bsize == target_bsize) {
        return if (status > 0) null else node;
    } else {
        if (status & 1 > 0) return null;
        if (status & (0b10) > 0) {
            // Internal
            var child = Block{
                .depth = node.depth + 1,
                .col = node.col * 2,
                .bsize = node.bsize / 2,
            };
            const block1 = self.alloc_traverse(child, target_bsize);
            if (block1) |b1| {
                if (b1.bsize == target_bsize) return b1;
            }
            child.col += 1;
            const block2 = self.alloc_traverse(child, target_bsize);
            if (block1) |b1| {
                if (block2) |b2| {
                    return if (b2.bsize < b1.bsize) b2 else b1;
                } else return b1;
            }
            return block2;
        }
        return node;
    }
}

fn split_block(self: *Self, block: Block, target_bsize: usize) Block {
    if (block.bsize == target_bsize) return block;
    self.set_status(block, 0b10);
    var child = Block{
        .bsize = block.bsize / 2,
        .depth = block.depth + 1,
        .col = block.col * 2 + 1,
    };
    self.set_status(child, 0);
    child.col -= 1;
    return self.split_block(child, target_bsize);
}

fn merge_buddies(self: *Self, block: Block) void {
    var buddy = block;
    buddy.col ^= 1;
    if (buddy.depth > 0 and self.get_status(buddy) == 0) {
        self.merge_buddies(Block{
            .depth = block.depth - 1,
            .col = block.col / 2,
            .bsize = block.bsize / 2,
        });
    } else {
        self.set_status(block, 0);
    }
}

fn block_addr(self: Self, block: Block) usize {
    return @intFromPtr(self.bitmap.ptr) + self.bitmap.len + block.bsize * block.col;
}

fn get_status(self: Self, block: Block) u2 {
    const d = self.translate(block);
    return @truncate(self.bitmap[d[0]] >> d[1]);
}

fn set_status(self: *Self, block: Block, status: u2) void {
    const d = self.translate(block);
    self.bitmap[d[0]] &= ~(d[2] << d[1]);
    self.bitmap[d[0]] |= @as(u8, @intCast(status)) << d[1];
}

fn translate(self: Self, block: Block) Tuple(&.{ usize, u3, u8 }) {
    // 2 ^ 0 = 1, so subtract one to be zero indexed
    const num_blocks = math.pow(usize, 2, block.depth) + block.col - 1;
    var bit_off: usize = undefined;
    var mask: u2 = undefined;
    if (num_blocks < self.internal_blocks) {
        bit_off = num_blocks * 2;
        mask = 0b11;
    } else {
        bit_off = self.internal_blocks * 2 + (num_blocks - self.internal_blocks);
        mask = 1;
    }
    return .{ bit_off / 8, @intCast(bit_off % 8), mask };
}

fn calc_split(_size: usize) Tuple(&.{ usize, usize, usize, usize }) {
    const size = _size / 0x1000 * 0x1000;
    var free_size = size - 0x1000;
    var max_bsize = math.ceilPowerOfTwoAssert(usize, free_size);
    var meta_ib = get_meta_size(max_bsize);
    while (meta_ib[1] + free_size > size) {
        free_size -= 0x1000;
        max_bsize = math.ceilPowerOfTwoAssert(usize, free_size);
        meta_ib = get_meta_size(max_bsize);
    }
    return .{
        max_bsize,
        free_size,
        meta_ib[1],
        meta_ib[0],
    };
}

fn get_meta_size(bsize: usize) Tuple(&.{ usize, usize }) {
    const internal_blocks = math.pow(usize, 2, math.log2(bsize / 0x1000));
    return .{ internal_blocks, math.divCeil(usize, 2 * internal_blocks + bsize / 0x1000, 8) catch unreachable };
}
