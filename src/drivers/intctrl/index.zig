pub const ioapic = @import("ioapic.zig");

const kernel = @import("root");
const mem = kernel.lib.mem;

pub const VTable = struct {
    map: *const fn (vec: u8, irq: usize) error{ IRQUsed, OutOfIRQs, InvalidIRQ }!usize,
    unmap: *const fn (irq: usize) void,
    mask: *const fn (irq: usize, m: bool) void,
    pref_vec: *const fn (irq: usize) ?u8,
    eoi: *const fn (irq: usize) void,
};

pub var controller: ?*const VTable = null;

const AVAILABLE_CONTROLLERS = [_]type{
    ioapic,
};

pub var Task = kernel.Task{
    .name = "Interrupt Controller",
    .init = init,
    .dependencies = &.{},
};

fn init() kernel.Task.Ret {
    inline for (AVAILABLE_CONTROLLERS) |cntrl| {
        cntrl.InterruptControllerTask.run();
        if (cntrl.InterruptControllerTask.ret == .success) {
            controller = &cntrl.InterruptControllerVTable;
            break;
        }
    }
    if (controller == null) return .failed;
    asm volatile ("sti");
    return .success;
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
