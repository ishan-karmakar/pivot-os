/// Implementation of the buddy allocator (https://en.wikipedia.org/wiki/Buddy_memory_allocation)
/// Metadata is stored external to the arena, so size must be known at initialization
/// Uses an efficient bitmap under the hood to save space
const std = @import("std");
const Tuple = std.meta.Tuple;
const log = std.log.scoped(.vmm);
const math = std.math;
const Allocator = std.mem.Allocator;
const mem = @import("kernel").lib.mem;
const Self = @This();
const Block = struct {
    depth: usize,
    col: usize,
    bsize: usize,
};

// FIXME: Actually think about all these divCeil catch unreachables

bitmap: []u8,
internal_blocks: usize,
max_bsize: usize,
mapper: *mem.Mapper,
flags: u64,

pub fn create(start: usize, size: usize, flags: u64, mapper: *mem.Mapper) Self {
    const sizes = calc_split(size);
    log.debug("Start: 0x{x}, Max block size: {}, Free size: {}, Metadata size: {}", .{ start, sizes[0], sizes[1], sizes[2] });
    // TODO: is meta size guaranteed to be page size multiple?
    const pages = math.divCeil(usize, sizes[2], 0x1000) catch unreachable;
    for (0..pages) |i| {
        const frm = mem.pmm.frame();
        mapper.map(frm, start + i * 0x1000, flags | 0b10 | (1 << 63));
    }
    const bitmap = @as([*]u8, @ptrFromInt(start))[0..(pages * 0x1000)];
    @memset(bitmap, 0);
    // reserve extra areas
    return Self{
        .bitmap = bitmap,
        .flags = flags,
        .internal_blocks = sizes[3],
        .mapper = mapper,
        .max_bsize = sizes[0],
    };
}

// pub fn allocator(self: *Self) Allocator {
//     return .{
//         .ptr = self,
//         .vtable = &.{},
//     };
// }

pub fn alloc(self: *Self, len: usize, ptr_align: u8, ret_addr: usize) ?[*]u8 {
    _ = ptr_align; // TODO: Handle alignment
    _ = ret_addr;
    const bsize = @min(0x1000, math.ceilPowerOfTwoAssert(usize, len));
    if (bsize > self.max_bsize) {
        return null;
    }
    const block = self.alloc_traverse(Block{
        .depth = 0,
        .col = 0,
        .bsize = self.max_bsize,
    }, bsize);
    if (block) |b| {
        const final_block = if (b.bsize == bsize) b else self.split_block(b, bsize);
        self.set_status(final_block, 1);
        log.debug("Using block {any}", .{final_block});
        const addr = @intFromPtr(self.bitmap.ptr) + self.bitmap.len + final_block.bsize * final_block.col;
        log.debug("Allocation address: 0x{x}", .{addr});
        for (0..math.divCeil(usize, len, 0x1000) catch unreachable) |i| {
            const frm = mem.pmm.frame();
            self.mapper.map(frm, addr + i * 0x1000, self.flags);
        }
        return @ptrFromInt(addr);
    }
    return null;
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
