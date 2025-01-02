const limine = @import("limine");
const ssfn = @import("ssfn");
const kernel = @import("kernel");
const std = @import("std");
const log = std.log.scoped(.fb);
const mem = kernel.lib.mem;

pub var Task = kernel.Task{
    .name = "Framebuffer (SSFN)",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.modules.Task },
        .{ .task = &mem.KHeapTask },
    },
};

export var FB_REQUEST = limine.FramebufferRequest{ .revision = 3 };

pub fn init() bool {
    // const response = FB_REQUEST.response orelse return false;
    // const fb = response.framebuffers()[0];
    // const buf = ssfn.ssfn_buf_t{
    //     .ptr = fb.address,
    //     .w = @intCast(fb.width),
    //     .h = @intCast(fb.height),
    //     .p = @intCast(fb.pitch),
    //     .x = 100,
    //     .y = 100,
    //     .fg = 0xFF808080,
    // };

    const ctx = mem.kheap.allocator().create(ssfn.ssfn_t) catch @panic("OOM");
    ctx.* = .{};

    const font = kernel.drivers.modules.get_module("font");
    if (ssfn.ssfn_load(ctx, @ptrFromInt(font)) != ssfn.SSFN_OK) {
        ssfn.ssfn_free(ctx);
        return false;
    }
    return true;
}

export fn realloc(ptr: ?*anyopaque, size: usize) ?*anyopaque {
    if (ptr != null) {
        log.info("OLD PTR: 0x{x}", .{ptr.?});
        @panic("realloc called with non null pointer");
    }
    // Free does not give us the size of the allocation
    // Instead we have to encode it in the allocation itself.
    // We allocate size + sizeof(usize) bytes and encode size of total allocation at the front
    const alloc_size = size + @sizeOf(usize);
    const alloc = @intFromPtr((mem.kheap.allocator().alignedAlloc(u8, @alignOf(usize), alloc_size) catch @panic("OOM")).ptr);
    @as(*usize, @ptrFromInt(alloc)).* = alloc_size;
    return @ptrFromInt(alloc + @sizeOf(usize));
}

export fn free(_ptr: ?*anyopaque) void {
    _ = _ptr;
    @panic("free called");
    // const ptr = _ptr orelse @panic("Free called with null pointer");
    // const start = @intFromPtr(ptr) - @sizeOf(usize);
    // const size = @as(*const usize, @ptrFromInt(start)).*;
    // const area = @as([*]const u8, @ptrFromInt(start))[0..size];
    // mem.kheap.allocator().free(area);
    // const size = @as(*const usize, @ptrFromInt(@intFromPtr(ptr) - @sizeOf(usize))).*;
}
