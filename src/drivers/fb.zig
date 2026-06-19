const limine = @import("limine");
const ssfn = @import("ssfn");
const kernel = @import("root");
const std = @import("std");
const log = std.log.scoped(.fb);
const mem = kernel.lib.mem;

export var FB_REQUEST = limine.limine_framebuffer_request{
    .id = kernel.LIMINE_REQUEST_ID(0x9d5827dcd881dd75, 0xa3148604f6fab11b),
    .revision = 1,
};

var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "Framebuffer");
    kernel.drivers.modules.init() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Framebuffer", err);
    const response: *limine.limine_framebuffer_response = FB_REQUEST.response orelse
        return kernel.lib.logger.failed_initialization(log, "Framebuffer", error.FramebufferUnavailable);
    // TODO: Multiple framebuffers
    const fb: *limine.limine_framebuffer = response.framebuffers[0];
    const font = kernel.drivers.modules.get_module("font") catch |err|
        return kernel.lib.logger.failed_initialization(log, "Framebuffer", err);

    ssfn.ssfn_src = @ptrFromInt(font);
    ssfn.ssfn_dst.bg = 0;
    ssfn.ssfn_dst.fg = 0xFFFFFFFF;
    ssfn.ssfn_dst.x = 0;
    ssfn.ssfn_dst.y = 0;
    ssfn.ssfn_dst.h = @intCast(fb.height);
    ssfn.ssfn_dst.w = @intCast(fb.width);
    ssfn.ssfn_dst.p = @intCast(fb.pitch);
    ssfn.ssfn_dst.ptr = @ptrCast(fb.address);
    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "Framebuffer");
}

pub fn write(bytes: []const u8) !void {
    if (!initialized)
        return error.FramebufferNotInitialized;
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
