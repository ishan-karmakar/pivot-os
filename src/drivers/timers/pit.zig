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

var triggered: bool = false;

pub fn init() void {
    idt.set_ent(VEC, idt.create_irq(VEC, "pit_handler"));
    serial.out(0x43, @as(u8, 0x34));
    if (!intctrl.set(VEC, IRQ, 0)) {
        @panic("Interrupt controller failed to set");
    }
}

pub fn sleep(ms: u16) void {
    const d = ms * MS_TICKS;
    serial.out(DATA_REG, @as(u8, @truncate(d)));
    serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    intctrl.mask(IRQ, false);
    while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    intctrl.mask(IRQ, true);
}

export fn pit_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    @atomicStore(bool, &triggered, true, .unordered);
    intctrl.eoi(IRQ);
    return status;
}
