/// Implementation of the buddy allocator (https://en.wikipedia.org/wiki/Buddy_memory_allocation)
/// Metadata is stored external to the arena, so size must be known at initialization
/// Uses an efficient bitmap under the hood to save space
const std = @import("std");
const Tuple = std.meta.Tuple;
const log = std.log.scoped(.vmm);
const math = std.math;
const mem = @import("kernel").lib.mem;
const Self = @This();

bitmap: []u8,
internal_blocks: usize,
max_bsize: usize,
mapper: *mem.Mapper,
flags: u64,

pub fn create(start: usize, size: usize, flags: u64, mapper: *mem.Mapper) Self {
    const sizes = calc_split(size);
    log.debug("Max block size: {}, Free size: {}, Metadata size: {}", .{ sizes[0], sizes[1], sizes[2] });
    const pages = math.divCeil(usize, sizes[2], 0x1000) catch unreachable;
    for (0..pages) |i| {
        const frm = mem.pmm.frame();
        mapper.map(frm, start + i * 0x1000, flags | 0b10 | (1 << 63));
    }
    const bitmap: [*]u8 = @ptrFromInt(start);
    // memset to zero
    return Self{
        .bitmap = bitmap[0..sizes[2]],
        .flags = flags,
        .internal_blocks = sizes[3],
        .mapper = mapper,
        .max_bsize = sizes[0],
    };
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
