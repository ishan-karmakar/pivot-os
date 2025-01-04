pub const ioapic = @import("ioapic.zig");
pub const pic = @import("pic.zig");

const kernel = @import("kernel");
const mem = kernel.lib.mem;

pub const VTable = struct {
    map: *const fn (self: *const @This(), vec: u8, irq: usize) error{ IRQUsed, OutOfIRQs, InvalidIRQ }!usize,
    unmap: *const fn (self: *const @This(), irq: usize) void,
    mask: *const fn (self: *const @This(), irq: usize, m: bool) void,
    pref_vec: *const fn (self: *const @This(), irq: usize) ?u8,
    eoi: *const fn (self: *const @This(), irq: usize) void,
};

pub var controller: ?*const VTable = null;

pub var Task = kernel.Task{
    .name = "Interrupt Controller",
    .init = init,
    .dependencies = &.{
        .{ .task = &ioapic.Task, .accept_failure = true },
        .{ .task = &pic.Task, .accept_failure = true },
    },
};

fn init() kernel.Task.Ret {
    if (controller == null) return .failed;
    asm volatile ("sti");
    return .success;
}

pub inline fn map(vec: u8, irq: usize) !usize {
    return controller.?.map(controller.?, vec, irq);
}

pub inline fn unmap(irq: usize) void {
    return controller.?.unmap(controller.?, irq);
}

pub inline fn eoi(irq: usize) void {
    return controller.?.eoi(controller.?, irq);
}

pub inline fn mask(irq: usize, m: bool) void {
    return controller.?.mask(controller.?, irq, m);
}

pub inline fn pref_vec(irq: usize) ?u8 {
    return controller.?.pref_vec(controller.?, irq);
}
