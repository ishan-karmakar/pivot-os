const pic = @import("kernel").drivers.pic;
const idt = @import("kernel").drivers.idt;
const timers = @import("kernel").drivers.timers;
const serial = @import("kernel").drivers.serial;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const IRQ = 0;
const MS_TICKS = 1193;

pub fn init() void {
    idt.set_ent(&idt.table[0x20 + IRQ], idt.create_irq(false, "pit_handler"));
    serial.out(CMD_REG, @as(u8, 0x34));
}

pub fn start(ms: u16) void {
    const d = ms * MS_TICKS;
    log.info("Starting PIT timer to trigger every {} milliseconds", .{ms});
    serial.out(DATA_REG, @as(u8, @truncate(d)));
    serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    pic.unmask(IRQ);
}

pub fn stop() void {
    start(0);
    pic.mask(IRQ);
}

export fn pit_handler() void {
    _ = @atomicRmw(usize, &timers.ticks, .Add, 1, .acq_rel);
    pic.eoi(IRQ);
}
