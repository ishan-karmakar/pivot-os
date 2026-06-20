const kernel = @import("root");
const log = @import("std").log.scoped(.intctrl);
const mem = kernel.lib.mem;

pub const InterruptController = struct {
    rating: u16,
    map: *const fn (vec: u8, irq: usize) error{ IRQUsed, OutOfIRQs, InvalidIRQ }!usize,
    unmap: *const fn (irq: usize) void,
    mask: *const fn (irq: usize, m: bool) void,
    pref_vec: *const fn (irq: usize) ?u8,
    eoi: *const fn (irq: usize) void,
};

var controller: ?*const InterruptController = null;

pub fn init() !void {
    @import("ioapic.zig").init() catch {};
    if (controller == null)
        return kernel.lib.logger.failed_initialization(log, "Interrupt Controller Subsystem", error.NoInterruptControllersAvailable);
    asm volatile ("sti");
    return kernel.lib.logger.successfully_initialized(log, "Interrupt Controller Subsystem");
}

pub fn register_controller(c: *const InterruptController) void {
    if (controller == null or c.rating > controller.?.rating)
        controller = c;
}

pub inline fn map(vec: u8, irq: usize) !usize {
    return controller.?.map(vec, irq);
}

pub inline fn unmap(irq: usize) void {
    return controller.?.unmap(irq);
}

pub inline fn eoi(irq: usize) void {
    return controller.?.eoi(irq);
}

pub inline fn mask(irq: usize, m: bool) void {
    return controller.?.mask(irq, m);
}

pub inline fn pref_vec(irq: usize) ?u8 {
    return controller.?.pref_vec(irq);
}
