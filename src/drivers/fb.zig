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
    },
};

export var FB_REQUEST = limine.FramebufferRequest{ .revision = 3 };

pub fn init() kernel.Task.Ret {
    const response = FB_REQUEST.response orelse return .failed;
    // TODO: Multiple framebuffers
    const fb = response.framebuffers()[0];
    const font = kernel.drivers.modules.get_module("font");

    ssfn.ssfn_src = @ptrFromInt(font);
    ssfn.ssfn_dst.bg = 0;
    ssfn.ssfn_dst.fg = 0xFFFFFFFF;
    ssfn.ssfn_dst.x = 0;
    ssfn.ssfn_dst.y = 0;
    ssfn.ssfn_dst.h = @intCast(fb.height);
    ssfn.ssfn_dst.w = @intCast(fb.width);
    ssfn.ssfn_dst.p = @intCast(fb.pitch);
    ssfn.ssfn_dst.ptr = fb.address;
    return .success;
}

pub fn write(bytes: []const u8) void {
    if ((Task.ret orelse return) != .success) return;
    for (bytes) |b| {
        // Wrap around on x overflow
        if (ssfn.ssfn_dst.x >= ssfn.ssfn_dst.w) {
            ssfn.ssfn_dst.y += ssfn.ssfn_src.*.height;
            ssfn.ssfn_dst.x = 0;
        }

        // Clear screen on y overflow
        if (ssfn.ssfn_dst.y >= ssfn.ssfn_dst.h) {
            ssfn.ssfn_dst.y = 0;
            @memset(ssfn.ssfn_dst.ptr[0..(@as(usize, @intCast(ssfn.ssfn_dst.p)) * @as(usize, @intCast(ssfn.ssfn_dst.h)))], 0);
        }
        if (b == '\n') {
            ssfn.ssfn_dst.x = 0;
            ssfn.ssfn_dst.y += ssfn.ssfn_src.*.height;
        } else if (ssfn.ssfn_putc(@intCast(b)) != ssfn.SSFN_OK) @panic("ssfn_putc failed");
    }
}
