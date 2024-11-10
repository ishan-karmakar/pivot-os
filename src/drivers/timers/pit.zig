const pic = @import("kernel").drivers.pic;
const idt = @import("kernel").drivers.idt;
const timers = @import("kernel").drivers.timers;
const serial = @import("kernel").drivers.serial;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const IRQ = 0;
const MS_TICKS = 1193;

var triggered: bool = false;

pub fn init() void {
    idt.set_ent(0x20 + IRQ, idt.create_irq(0, "pit_handler"));
    serial.out(CMD_REG, @as(u8, 0x34));
}

pub fn sleep(ms: u16) void {
    const d = ms * MS_TICKS;
    serial.out(DATA_REG, @as(u8, @truncate(d)));
    serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    pic.unmask(IRQ);
    while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    pic.mask(IRQ);
}

export fn pit_handler(status: *const idt.Status, _: usize) *const idt.Status {
    @atomicStore(bool, &triggered, true, .unordered);
    pic.eoi(IRQ);
    return status;
}
