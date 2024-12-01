// Interrupt Controllers
pub const ioapic = @import("ioapic.zig");
pub const pic = @import("pic.zig");
const log = @import("std").log.scoped(.intctrl);

pub const VTable = struct {
    init: *const fn () bool,
    set: *const fn (vec: u8, irq: u5, flags: u64) void,
    mask: *const fn (irq: u5, m: bool) void,
    eoi: *const fn (irq: u5) void,
};

var controller: *const VTable = undefined;

pub fn init() void {
    if (ioapic.vtable.init()) {
        controller = &ioapic.vtable;
    } else if (pic.vtable.init()) {
        controller = &pic.vtable;
    } else @panic("No interrupt controller available");
}

/// We are relying on the assumption that mask is only called after set, and mask won't be called if set fails
pub fn mask(irq: u5, m: bool) void {
    return controller.mask(irq, m);
}

pub fn set(vec: u8, irq: u5, flags: u64) void {
    return controller.set(vec, irq, flags);
}

/// We are relying on the assumption that eoi is only called after set, and eoi won't be called if set fails
pub fn eoi(irq: u5) void {
    return controller.eoi(irq);
}
