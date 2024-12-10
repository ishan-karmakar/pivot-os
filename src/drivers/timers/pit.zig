const kernel = @import("kernel");
const intctrl = kernel.drivers.intctrl;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const timers = kernel.drivers.timers;
const serial = kernel.drivers.serial;
const math = @import("std").math;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const VEC = 0x20;
const IRQ = 0;
const HZ = 1193182;

pub const vtable: timers.VTable = .{
    .capabilities = timers.CAPABILITIES_IRQ,
    .init = init,
    .time = null,
    .sleep = sleep,
};

var vector: u8 = undefined;
var initialized: bool = false;

pub fn init() bool {
    if (initialized) return true;
    initialized = true;
    vector = idt.allocate_vec(VEC, timer_handler, null) orelse @panic("Out of interrupt handlers");
    intctrl.set(vector, IRQ, 0);
    serial.out(CMD_REG, @as(u8, 0x30));
    log.info("Initialized PIT", .{});
    return true;
}

fn raw_sleep(ticks: u16) void {
    var triggered = false;
    idt.set_ctx(vector, &triggered);
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.mask(IRQ, false);
    while (!@atomicLoad(bool, @as(*volatile bool, &triggered), .unordered)) asm volatile ("pause");
    intctrl.mask(IRQ, true);
    log.info("end of sleep", .{});
}

pub fn sleep(ns: usize) void {
    // Simple cross multiplying to get number of ticks
    // ns / 1 second = ? ticks / hz
    const ticks = @max(1, ns * HZ / 1_000_000_000);
    const overflows = @divFloor(ticks, 0xFF);
    const remainder = ticks % 0xFF;
    for (0..overflows) |_| raw_sleep(0xFF);
    raw_sleep(@intCast(remainder));
    // const d = ms * MS_TICKS;
    // serial.out(DATA_REG, @as(u8, @truncate(d)));
    // serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    // intctrl.mask(IRQ, false);
    // while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    // intctrl.mask(IRQ, true);
}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    log.info("timer handler", .{});
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    intctrl.eoi(IRQ);
    return status;
}
