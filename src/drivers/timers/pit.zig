const pic = @import("kernel").drivers.pic;
const idt = @import("kernel").drivers.idt;
const cpu = @import("kernel").drivers.cpu;
const timers = @import("kernel").drivers.timers;
const serial = @import("kernel").drivers.serial;
const ioapic = @import("kernel").drivers.ioapic;
const lapic = @import("kernel").drivers.lapic;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const VEC = 0x20;
const MS_TICKS = 1193;

var triggered: bool = false;

pub fn init() void {
    idt.set_ent(VEC, idt.create_irq(VEC, "pit_handler"));
    serial.out(0x43, @as(u8, 0x34));
}

pub fn sleep(ms: u16) void {
    const d = ms * MS_TICKS;
    serial.out(DATA_REG, @as(u8, @truncate(d)));
    serial.out(DATA_REG, @as(u8, @truncate(d >> 8)));
    // pic.unmask(VEC - 0x20);
    while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    // pic.mask(VEC - 0x20);
}

export fn pit_handler(status: *const cpu.Status, _: usize) *const cpu.Status {
    log.info("pit_handler", .{});
    // @atomicStore(bool, &triggered, true, .unordered);
    // lapic.eoi();
    return status;
}
