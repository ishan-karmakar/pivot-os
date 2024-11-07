const limine = @import("limine");
const flanterm = @import("flanterm");
const mem = @import("kernel").lib.mem;
const log = @import("std").log.scoped(.term);
const ArrayList = @import("std").ArrayList;
export var FB_REQUEST: limine.FramebufferRequest = .{};

// TODO: Consider ArrayListUnmanaged then no need for optional
var main_term: [*c]flanterm.flanterm_context = undefined;
// var terms: ?ArrayList([*c]flanterm.struct_flanterm_context) = null;

pub fn init() void {
    const framebuffers = (FB_REQUEST.response orelse @panic("Limine FB request is null"));
    // terms = ArrayList([*c]flanterm.struct_flanterm_context).init(mem.kheap.allocator());
    // for (0..framebuffers.framebuffer_count) |i| {
    const fb = framebuffers.framebuffers_ptr[0];
    main_term = flanterm.flanterm_fb_init(
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
    // terms.?.append(ctx) catch @panic("Error adding terminal context");
    // }
    log.info("Initialized terminals", .{});
}

pub fn write(bytes: []const u8) !usize {
    // if (terms) |term| {
    // for (term.items) |t| {
    flanterm.flanterm_write(main_term, bytes.ptr, bytes.len);
    // }
    // }
    return bytes.len;
}
