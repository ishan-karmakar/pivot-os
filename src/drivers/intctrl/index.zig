pub const ioapic = @import("ioapic.zig");
pub const pic = @import("pic.zig");

const kernel = @import("kernel");
const mem = kernel.lib.mem;

pub const VTable = struct {
    map: *const fn (vec: u8, irq: usize) error{ IRQUsed, OutOfIRQs, InvalidIRQ }!usize,
    unmap: *const fn (irq: usize) void,
    mask: *const fn (irq: usize, m: bool) void,
    eoi: *const fn (irq: usize) void,
};

pub var controller: *const VTable = undefined;

pub var Task = kernel.Task{
    .name = "Interrupt Controller",
    .init = init,
    .dependencies = &.{
        .{ .task = &ioapic.Task, .accept_failure = true },
    },
};

fn init() bool {
    if (ioapic.Task.ret.?) {
        controller = &ioapic.vtable;
    } else {
        pic.Task.run();
        if (pic.Task.ret.?) {
            controller = &pic.vtable;
        } else return false;
    }
    asm volatile ("sti");
    return true;
}
