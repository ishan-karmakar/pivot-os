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

pub const vtable = timers.VTable{
    .init = init,
    .time = null,
    .sleep = sleep,
};

var initialized: bool = false;
var irq: usize = undefined;

fn init() bool {
    if (initialized) return true;
    initialized = true;
    log.info("Initialized PIT", .{});
    sleep(1_000_000); // Sleep for 1 ms so no more interrupts fire
    return true;
}

fn raw_sleep(handler: *idt.HandlerData, ticks: u16) void {
    var triggered = false;
    handler.ctx = &triggered;
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.controller.mask(irq, false);
    while (!@atomicLoad(bool, @as(*volatile bool, &triggered), .unordered)) asm volatile ("pause");
    intctrl.controller.mask(irq, true);
}

fn sleep(ns: usize) void {
    const handler = idt.allocate_handler(IRQ + 0x20);
    // FIXME: Check vector for 8259 PIC
    handler.handler = timer_handler;
    irq = intctrl.controller.map(idt.handler2vec(handler), IRQ) catch @panic("Could not map PIT IRQ");

    // Command for oneshot
    serial.out(CMD_REG, @as(u8, 0x30)); // 0x30, 0x34
    // Simple cross multiplying to get number of ticks
    // ns / 1 second = ? ticks / hz
    const ticks = @max(1, ns * HZ / 1_000_000_000);
    const overflows = @divFloor(ticks, 0xFFFF);
    const remainder = ticks % 0xFFFF;
    for (0..overflows) |_| raw_sleep(handler, 0xFFFF);
    // FIXME: Panic if vector actually chosen is not suitable for 8259 PIC
    raw_sleep(handler, @intCast(remainder));
    handler.reserved = false;
    intctrl.controller.unmap(irq);
}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    intctrl.controller.eoi(irq);
    return status;
}
