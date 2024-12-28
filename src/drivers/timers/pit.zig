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

var handler: *idt.HandlerData = undefined;
var initialized: bool = false;

fn init() bool {
    if (initialized) return true;
    initialized = true;
    handler = idt.allocate_handler(null);
    handler.handler = timer_handler;
    handler.ctx = null;
    intctrl.set(idt.handler2vec(handler), IRQ, 0);
    log.info("Initialized PIT", .{});
    sleep(1_000_000); // Sleep for 1 ms so no more interrupts fire
    return true;
}

fn raw_sleep(ticks: u16) void {
    var triggered = false;
    handler.ctx = &triggered;
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.mask(IRQ, false);
    while (!@atomicLoad(bool, @as(*volatile bool, &triggered), .unordered)) asm volatile ("pause");
    intctrl.mask(IRQ, true);
}

fn sleep(ns: usize) void {
    // Command for oneshot
    serial.out(CMD_REG, @as(u8, 0x30)); // 0x30, 0x34
    // Simple cross multiplying to get number of ticks
    // ns / 1 second = ? ticks / hz
    const ticks = @max(1, ns * HZ / 1_000_000_000);
    const overflows = @divFloor(ticks, 0xFFFF);
    const remainder = ticks % 0xFFFF;
    for (0..overflows) |_| raw_sleep(0xFFFF);
    raw_sleep(@intCast(remainder));
}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    intctrl.eoi(IRQ);
    return status;
}
