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
    &ioapic.vtable,
    &pic.vtable,
};

var controller: ?*const VTable = null;

pub fn init() void {
    for (avail_controllers) |c| {
        if (c.init()) {
            log.debug("{s} completed initialization", .{c.name});
            controller = c;
            break;
        } else {
            log.warn("{s} failed initialization", .{c.name});
        }
    }
}

/// We are relying on the assumption that mask is only called after set, and mask won't be called if set fails
pub fn mask(irq: u5, m: bool) void {
    return controller.?.mask(irq, m);
}

pub fn set(vec: u8, irq: u5, flags: u64) bool {
    if (controller) |c| return c.set(vec, irq, flags);
    @panic("No active interrupt controller");
}

/// We are relying on the assumption that eoi is only called after set, and eoi won't be called if set fails
pub fn eoi(irq: u5) void {
    return controller.?.eoi(irq);
}
