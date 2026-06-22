const limine = @import("limine");
const flanterm = @import("flanterm");
const kernel = @import("root");
const std = @import("std");
const log = std.log.scoped(.fb);
const mem = kernel.mem;

export var FB_REQUEST = limine.limine_framebuffer_request{
    .id = kernel.LIMINE_REQUEST_ID(0x9d5827dcd881dd75, 0xa3148604f6fab11b),
    .revision = 1,
};

export var FLANTERM_INIT_PARAMS_REQUEST = limine.limine_flanterm_fb_init_params_request{
    .id = kernel.LIMINE_REQUEST_ID(0x3259399fe7c5f126, 0xe01c1c8c5db9d1a9),
};

var main_fb: ?*flanterm.flanterm_context = null;
var other_fbs = std.ArrayList(*flanterm.flanterm_context).empty;

/// Initializes the first framebuffer using the default bump allocator just to get a monitor working
pub fn init_main() !void {
    if (main_fb != null) return;

    if (FB_REQUEST.response == null or FB_REQUEST.response.*.framebuffer_count == 0)
        return kernel.lib.logger.failed_initialization(log, "Framebuffer (Main)", error.FBUnavailable);

    if (FLANTERM_INIT_PARAMS_REQUEST.response == null)
        return kernel.lib.logger.failed_initialization(log, "Framebuffer (Main)", error.FBInitParamsUnavailable);

    const fb_response: *limine.limine_framebuffer_response = FB_REQUEST.response;
    const params_response: *limine.limine_flanterm_fb_init_params_response = FLANTERM_INIT_PARAMS_REQUEST.response;

    if (fb_response.framebuffer_count != params_response.entry_count)
        return kernel.lib.logger.failed_initialization(log, "Framebuffer (Main)", error.FBCountsNotMatching);

    const fb: *limine.limine_framebuffer = fb_response.framebuffers[0];
    const params: *limine.limine_flanterm_fb_init_params = params_response.entries[0];

    main_fb = flanterm.flanterm_fb_init(
        null,
        null,
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
        params.canvas,
        &params.ansi_colours,
        &params.ansi_bright_colours,
        &params.default_bg,
        &params.default_fg,
        &params.default_bg_bright,
        &params.default_fg_bright,
        params.font,
        params.font_width,
        params.font_height,
        params.font_spacing,
        params.font_scale_x,
        params.font_scale_y,
        params.margin,
        @intCast(params.rotation),
    );

    kernel.lib.logger.successfully_initialized(log, "Framebuffer (Main)");
}

/// Initializes the rest of the monitors using the regular kernel heap allocator
pub fn init_all() !void {
    if (other_fbs.items.len > 0) return;
    init_main() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Framebuffer (All)", err);
    mem.init_kheap() catch |err|
        return kernel.lib.logger.failed_initialization(log, "Framebuffer (All)", err);

    if (FB_REQUEST.response.*.framebuffer_count <= 1)
        return;

    for (1..FB_REQUEST.response.*.framebuffer_count) |i| {
        const fb: *limine.limine_framebuffer = FB_REQUEST.response.*.framebuffers[i];
        const params: *limine.limine_flanterm_fb_init_params = FLANTERM_INIT_PARAMS_REQUEST.response.*.entries[i];

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
            params.canvas,
            &params.ansi_colours,
            &params.ansi_bright_colours,
            &params.default_bg,
            &params.default_fg,
            &params.default_bg_bright,
            &params.default_fg_bright,
            params.font,
            params.font_width,
            params.font_height,
            params.font_spacing,
            params.font_scale_x,
            params.font_scale_y,
            params.margin,
            @intCast(params.rotation),
        );
        if (ctx) |c| {
            other_fbs.append(mem.kheap.allocator(), c) catch |err|
                return kernel.lib.logger.failed_initialization(log, "Framebuffer (All)", err);
        } else log.warn("Flanterm failed to create context, skipping to next FB", .{});
    }
    kernel.lib.logger.successfully_initialized(log, "Framebuffer (All)");
}

pub fn write(bytes: []const u8) !void {
    if (main_fb == null)
        return error.FramebufferNotInitialized;

    flanterm.flanterm_write(main_fb, bytes.ptr, bytes.len);
}

fn malloc(size: usize) callconv(.c) ?*anyopaque {
    return (mem.kheap.allocator().alloc(u8, size) catch @panic("OOM")).ptr;
}

fn free(ptr: ?*anyopaque, size: usize) callconv(.c) void {
    mem.kheap.allocator().free(@as([*]const u8, @ptrCast(ptr))[0..size]);
}
