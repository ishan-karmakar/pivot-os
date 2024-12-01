const kernel = @import("kernel");
const intctrl = kernel.drivers.intctrl;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const timers = kernel.drivers.timers;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const VEC = 0x20;
const IRQ = 0;
const MS_TICKS = 1193;

pub fn init() void {
    serial.out(0x43, @as(u8, 0x34));
    @panic("test");
}

pub fn sleep(ms: u16) void {
    _ = ms;
    // const d = ms * MS_TICKS;
    // serial.out(DATA_REG, @as(u8, @truncate(d)));
    // serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    // intctrl.mask(IRQ, false);
    // while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    // intctrl.mask(IRQ, true);
}

/// Sets the PIT to oneshot, handles the IRQ, then the PIT no longer triggers interrupts
pub fn disable() void {
    var triggered = false;
    const vec = idt.allocate_vec(VEC, timer_handler, &triggered) orelse @panic("Out of interrupt handlers");
    intctrl.set(vec, IRQ, 0);
    serial.out(CMD_REG, @as(u8, 0x30));
    serial.out(DATA_REG, @as(u8, @truncate(MS_TICKS)));
    serial.out(DATA_REG, @as(u8, @truncate(MS_TICKS >> 8)));
    intctrl.mask(IRQ, false);
    while (!triggered) asm volatile ("pause");
    intctrl.mask(IRQ, true);
    idt.free_vecs(vec, vec);
    log.debug("Disabled PIT", .{});
}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    intctrl.eoi(IRQ);
    return status;
}
