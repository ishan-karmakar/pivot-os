const limine = @import("limine");
const flanterm = @import("flanterm");
const kernel = @import("kernel");
const mem = kernel.lib.mem;
const log = @import("std").log.scoped(.term);
const ArrayList = @import("std").ArrayList;
const std = @import("std");
export var FB_REQUEST: limine.FramebufferRequest = .{};

var terms: []*flanterm.struct_flanterm_context = &.{};

pub var Task = kernel.Task{
    .name = "Framebuffer",
    .init = init,
    .dependencies = &.{
        .{ .task = &mem.KHeapTask },
    },
};

fn init() bool {
    const framebuffers = (FB_REQUEST.response orelse return false);
    var term_arr = ArrayList(*flanterm.struct_flanterm_context).init(mem.kheap.allocator());
    for (0..framebuffers.framebuffer_count) |i| {
        const fb = framebuffers.framebuffers_ptr[i];
        const ctx = flanterm.flanterm_fb_init(
            malloc,
            free,
            @ptrCast(@alignCast(fb.address)),
            fb.width,
            fb.height,
            fb.pitch,
            fb.red_mask_size,
            fb.red_mask_shift,
            fb.green_mask_size,
            fb.green_mask_shift,
            fb.blue_mask_size,
            fb.blue_mask_shift,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            null,
            0,
            0,
            1,
            0,
            0,
            0,
        );
        term_arr.append(ctx) catch return false;
    }
    terms = term_arr.toOwnedSlice() catch return false;
    return true;
}

pub fn write(bytes: []const u8) !usize {
    if (terms.len == 0) return 0;
    for (terms) |t| flanterm.flanterm_write(t, bytes.ptr, bytes.len);
    return bytes.len;
}

fn malloc(size: usize) callconv(.C) ?*anyopaque {
    return @ptrCast(mem.kheap.allocator().alloc(u8, size) catch @panic("OOM"));
}

fn free(ptr: ?*anyopaque, size: usize) callconv(.C) void {
    const arr: [*]u8 = @ptrCast(ptr orelse @panic("Free called with NULL"));
    mem.kheap.allocator().free(arr[0..size]);
}
