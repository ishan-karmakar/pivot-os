const limine = @import("limine");
const flanterm = @import("c");
const mem = @import("kernel").lib.mem;
export var FB_REQUEST: limine.FramebufferRequest = .{};

pub fn init() void {
    const fb = FB_REQUEST.response.?.framebuffers_ptr[0];
    _ = flanterm.flanterm_fb_init(
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
}

fn malloc(size: usize) callconv(.C) ?*anyopaque {
    return @ptrCast(mem.kheap.allocator().alloc(u8, size) catch @panic("OOM"));
}

fn free(ptr: ?*anyopaque, size: usize) callconv(.C) void {
    const arr: [*]u8 = @ptrCast(ptr orelse @panic("Free called with NULL"));
    mem.kheap.allocator().free(arr[0..size]);
}
