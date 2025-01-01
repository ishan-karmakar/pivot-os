const kernel = @import("kernel");
const timers = kernel.drivers.timers;
const intctrl = kernel.drivers.intctrl;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const serial = kernel.drivers.serial;
const math = @import("std").math;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const IRQ = 0;
const HZ = 1193182;

pub const vtable = timers.TimerVTable{
    .deinit = deinit,
};

pub var Task = kernel.Task{
    .name = "PIT",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
        .{ .task = &kernel.drivers.intctrl.Task },
    },
};

var irq: usize = undefined;
var handler: *idt.HandlerData = undefined;

fn init() bool {
    handler = idt.allocate_handler(IRQ + 0x20);
    handler.handler = timer_handler;
    irq = intctrl.controller.map(idt.handler2vec(handler), IRQ) catch return false;
    _ = sleep(1_000_000); // Sleep for 1 ms so no more interrupts fire
    timers.set_timer(&vtable);
    return true;
}

fn deinit() void {
    intctrl.controller.unmap(irq);
    handler.reserved = false;
}

fn sleep(ns: usize) bool {
    // Command for oneshot
    const ticks = (ns * HZ) / 1_000_000_000;
    var triggered = false;
    handler.ctx = &triggered;
    serial.out(CMD_REG, @as(u8, 0x30)); // 0x30, 0x34
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.controller.mask(irq, false);
    while (!@atomicLoad(bool, @as(*volatile bool, &triggered), .unordered)) asm volatile ("pause");
    intctrl.controller.mask(irq, true);
    return true;
}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    intctrl.controller.eoi(irq);
    return status;
}
