const kernel = @import("root");
const mem = kernel.mem;
const std = @import("std");
const log = std.log.scoped(.slub);
const Allocator = std.mem.Allocator;

pub const Slab = struct {
    free_count: u32,
    total_count: u32,
    free_list: ?*FreeObject,

    next: ?*@This() = null,

    fn create(cache: *Cache) !*@This() {
        const pages = try mem.kvmm.allocator().alloc(u8, cache.slab_size);
        const slab: *Slab = @ptrCast(@alignCast(pages.ptr));
        const objects_start = std.mem.alignForward(usize, @intFromPtr(slab) + @sizeOf(Slab), cache.object_size);
        const usable = cache.slab_size - (objects_start - @intFromPtr(slab));
        const count = usable / cache.object_size;
        slab.* = .{
            .free_count = @intCast(count),
            .total_count = @intCast(count),
            .free_list = null,
        };
        const base: [*]u8 = @ptrFromInt(objects_start);
        var freelist: ?*FreeObject = null;

        var i: usize = 0;
        while (i < count) : (i += 1) {
            const obj = @as(*FreeObject, @ptrCast(@alignCast(base + i * cache.object_size)));
            obj.next = freelist;
            freelist = obj;
        }
        slab.free_list = freelist;
        return slab;
    }

    fn remove_from_list(self: *@This(), list: *?*Slab) void {
        var current = list.*;
        if (current == self) {
            list.* = self.next;
            self.next = null;
            return;
        }

        while (current) |node| {
            if (node.next == self) {
                node.next = self.next;
                self.next = null;
                return;
            }
            current = node.next;
        }
    }
};

const Cache = struct {
    object_size: usize,
    slab_size: usize,
    partial: ?*Slab = null,
    full: ?*Slab = null,

    lock: kernel.lib.Spinlock = .{},

    fn create(object_size: usize) @This() {
        const slab_size = @max(object_size * 4, 0x1000);
        return .{
            .object_size = object_size,
            .slab_size = slab_size,
        };
    }
};

const FreeObject = struct {
    next: ?*@This(),
};

const MIN_CACHE_SIZE = 8;
const MAX_CACHE_SIZE = 8192;
pub const NUM_CACHES = get_cache_idx(MAX_CACHE_SIZE) - get_cache_idx(MIN_CACHE_SIZE) + 1;

var caches = blk: {
    var arr: [NUM_CACHES]Cache = undefined;

    for (get_cache_idx(MIN_CACHE_SIZE)..(get_cache_idx(MAX_CACHE_SIZE) + 1)) |i|
        arr[i] = Cache.create(MIN_CACHE_SIZE << @intCast(i));

    break :blk arr;
};

pub fn allocator(self: *@This()) Allocator {
    return .{
        .ptr = self,
        .vtable = &.{
            .alloc = alloc,
            .free = free,
            .resize = resize,
            .remap = remap,
        },
    };
}

fn alloc(_: ?*anyopaque, size: usize, alignment: std.mem.Alignment, ret_addr: usize) ?[*]u8 {
    if (size > MAX_CACHE_SIZE)
        return mem.kvmm.allocator().rawAlloc(size, alignment, ret_addr);
    const idx = get_cache_idx(@max(size, alignment.toByteUnits()));

    asm volatile ("cli");
    defer asm volatile ("sti");
    const cpu_info = kernel.cpu.smp.cpu_info(null);
    if (cpu_info.slabs[idx]) |slab| {
        if (slab.free_list) |obj| {
            slab.free_list = obj.next;
            slab.free_count -= 1;
            return @ptrCast(obj);
        }
    }
    const cache = &caches[idx];
    cache.lock.acquire();
    defer cache.lock.release();

    if (cpu_info.slabs[idx]) |slab| {
        slab.next = cache.full;
        cache.full = slab;
        cpu_info.slabs[idx] = null;
    }

    var slab = cache.partial;
    if (slab) |s| {
        cache.partial = s.next;
        s.next = null;
    } else slab = Slab.create(cache) catch return null;

    cpu_info.slabs[idx] = slab;

    const obj = slab.?.free_list.?;
    slab.?.free_list = obj.next;
    slab.?.free_count -= 1;
    return @ptrCast(obj);
}

fn free(_: ?*anyopaque, data: []u8, alignment: std.mem.Alignment, _: usize) void {
    if (data.len > MAX_CACHE_SIZE)
        return mem.kvmm.allocator().free(data);

    const idx = get_cache_idx(@max(data.len, alignment.toByteUnits()));
    var cache = caches[idx];
    const slab: *Slab = @ptrFromInt(@intFromPtr(data.ptr) & ~(cache.slab_size - 1));
    const obj: *FreeObject = @ptrCast(@alignCast(data.ptr));

    asm volatile ("cli");
    defer asm volatile ("sti");
    const cpu_info = kernel.cpu.smp.cpu_info(null);

    if (slab == cpu_info.slabs[get_cache_idx(cache.object_size)]) {
        obj.next = slab.free_list;
        slab.free_list = obj;
        slab.free_count += 1;
        return;
    }

    cache.lock.acquire();
    defer cache.lock.release();

    obj.next = slab.free_list;
    slab.free_list = obj;
    slab.free_count += 1;

    if (slab.free_count == slab.total_count) {
        slab.remove_from_list(&cache.partial);
        slab.remove_from_list(&cache.full);
        const slab_data: []u8 = @as([*]u8, @ptrCast(slab))[0..cache.slab_size];
        mem.kvmm.allocator().free(slab_data);
    } else if (slab.free_count == 1) {
        slab.remove_from_list(&cache.full);
        slab.next = cache.partial;
        cache.partial = slab;
    }
}

fn resize(_: ?*anyopaque, buf: []u8, alignment: std.mem.Alignment, new_size: usize, ret_addr: usize) bool {
    if (buf.len > MAX_CACHE_SIZE)
        return mem.kvmm.allocator().rawResize(buf, alignment, new_size, ret_addr);

    if (new_size > MAX_CACHE_SIZE)
        return false;

    const old_idx = get_cache_idx(@max(buf.len, alignment.toByteUnits()));
    const new_idx = get_cache_idx(@max(new_size, alignment.toByteUnits()));

    return old_idx == new_idx;
}

fn remap(_: ?*anyopaque, buf: []u8, alignment: std.mem.Alignment, new_size: usize, ret_addr: usize) ?[*]u8 {
    if (buf.len > MAX_CACHE_SIZE)
        return mem.kvmm.allocator().rawRemap(buf, alignment, new_size, ret_addr);

    if (resize(null, buf, alignment, new_size, ret_addr))
        return buf.ptr;
    return null;
}

inline fn get_cache_idx(size: usize) usize {
    if (size <= MIN_CACHE_SIZE) return 0;
    return std.math.log2_int_ceil(usize, size) - std.math.log2(MIN_CACHE_SIZE);
}
