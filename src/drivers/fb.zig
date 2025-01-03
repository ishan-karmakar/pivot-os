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

var buf: ssfn.ssfn_buf_t = undefined;
// Context is extremely large so I am using kernel heap
var ctx: *ssfn.ssfn_t = undefined;

pub fn init() bool {
    const allocator = mem.kheap.allocator();
    const response = FB_REQUEST.response orelse return false;
    const fb = response.framebuffers()[0];
    buf = .{
        .ptr = @ptrFromInt(@intFromPtr(fb.address) + 5 * 4), // Margin of 5
        .w = @intCast(fb.width - 5),
        .h = @intCast(fb.height),
        .p = @intCast(fb.pitch),
        .x = 0,
        .y = 16,
        .fg = 0xFFFFFFFF,
    };

    ctx = allocator.create(ssfn.ssfn_t) catch @panic("OOM");
    ctx.* = .{};

    const font = kernel.drivers.modules.get_module("font");
    var ret = ssfn.ssfn_load(ctx, @ptrFromInt(font));
    if (ret != ssfn.SSFN_OK) {
        ssfn.ssfn_free(ctx);
        allocator.destroy(ctx);
        return false;
    }

    ret = ssfn.ssfn_select(
        ctx,
        ssfn.SSFN_FAMILY_ANY,
        null,
        ssfn.SSFN_STYLE_REGULAR,
        16,
    );
    if (ret != ssfn.SSFN_OK) {
        ssfn.ssfn_free(ctx);
        allocator.destroy(ctx);
        return false;
    }

    return true;
}

pub fn write(str: []const u8) void {
    var i: usize = 0;
    while (true) {
        const ret = ssfn.ssfn_render(ctx, &buf, str[i..].ptr);
        if (ret < 0) {
            @panic("ssfn_render failed");
        } else if (ret == 0) {
            return;
        } else {
            i += @intCast(ret);
        }
    }
}

inline fn get_alloc_size(ptr: *anyopaque) usize {
    return @as(*const usize, @ptrFromInt(@intFromPtr(ptr) - @sizeOf(usize))).*;
}

export fn realloc(ptr: ?*anyopaque, size: usize) ?*anyopaque {
    if (ptr) |old_ptr| {
        // Not sure if it expects us to copy but I don't want to try debugging problems with that
        const new_ptr = realloc(null, size).?;
        const old_size = get_alloc_size(old_ptr);
        const old_buf = @as([*]const u8, @ptrCast(old_ptr))[0..old_size];
        const new_buf = @as([*]u8, @ptrCast(new_ptr))[0..old_size];
        @memcpy(new_buf, old_buf);
        free(old_ptr);
        return new_ptr;
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
    const ptr = _ptr orelse @panic("Free called with null pointer");
    const start = @intFromPtr(ptr) - @sizeOf(usize);
    const area = @as([*]const u8, @ptrFromInt(start))[0..get_alloc_size(ptr)];
    mem.kheap.allocator().free(area);
}
