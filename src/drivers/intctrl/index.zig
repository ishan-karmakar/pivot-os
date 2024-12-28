pub const ioapic = @import("ioapic.zig");
pub const pic = @import("pic.zig");

const kernel = @import("kernel");
const mem = kernel.lib.mem;

pub const VTable = struct {
    init: *const fn () bool,
    map: *const fn (vec: u8, irq: usize) error{ IRQUsed, OutOfIRQs, InvalidIRQ }!usize,
    unmap: *const fn (irq: usize) void,
    mask: *const fn (irq: usize, m: bool) void,
    eoi: *const fn (irq: usize) void,
};

pub var controller: *const VTable = undefined;

pub fn init() void {
    if (ioapic.vtable.init()) {
        controller = &ioapic.vtable;
    } else if (pic.vtable.init()) {
        controller = &pic.vtable;
    }
}
