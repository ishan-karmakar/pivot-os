/// Implementation of the buddy allocator (https://en.wikipedia.org/wiki/Buddy_memory_allocation)
/// Metadata is stored external to the arena, so size must be known at initialization
/// Uses an efficient bitmap under the hood to save space
const std = @import("std");
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
    log.debug("Max block size: {}, Free size: {}, Metadata size: {}", .{ sizes.max_bsize, sizes.free_size, sizes.meta_size });
    _ = start;
    _ = flags;
    _ = mapper;
}

fn calc_split(_size: usize) struct { max_bsize: usize, meta_size: usize, free_size: usize } {
    const size = _size / 0x1000 * 0x1000;
    var free_size = size - 0x1000;
    var max_bsize = math.ceilPowerOfTwoAssert(usize, free_size);
    var meta_size = get_meta_size(max_bsize);
    while (meta_size + free_size > size) {
        free_size -= 0x1000;
        max_bsize = math.ceilPowerOfTwoAssert(usize, free_size);
        meta_size = get_meta_size(max_bsize);
    }
    return .{
        .max_bsize = max_bsize,
        .meta_size = meta_size,
        .free_size = free_size,
    };
}

fn get_meta_size(bsize: usize) usize {
    const internal_blocks = math.pow(usize, 2, math.log2(bsize / 0x1000));
    return math.divCeil(usize, 2 * internal_blocks + bsize / 0x1000, 8) catch unreachable;
}
