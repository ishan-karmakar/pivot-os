// Interrupt Controllers
pub const ioapic = @import("ioapic.zig");
pub const pic = @import("pic.zig");
const log = @import("std").log.scoped(.intctrl);

pub const VTable = struct {
    name: []const u8,
    init: *const fn () bool,
    set: *const fn (vec: u8, irq: u5, flags: u64) bool,
    mask: *const fn (irq: u5, m: bool) void,
    eoi: *const fn (irq: u5) void,
};

const avail_controllers = [_]*const VTable{
    &pic.vtable,
    &ioapic.vtable,
};

var controller: ?*const VTable = null;

pub fn init() void {
    for (avail_controllers) |c| {
        if (c.init()) {
            log.debug("{s} completed initialization", .{c.name});
            controller = c;
        } else {
            log.warn("{s} failed initialization", .{c.name});
        }
    }
    if (controller) |c| {
        log.info("Active interrupt controller: {s}", .{c.name});
    } else {
        log.warn("No active interrupt controller", .{});
    }
}
