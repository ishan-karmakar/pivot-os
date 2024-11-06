const lapic = @import("kernel").drivers.lapic;
const timers = @import("kernel").drivers.timers;
const log = @import("std").log.scoped(.lapic);
const idt = @import("kernel").drivers.idt;

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;
const EOI_OFF = 0xB0;
const LVT_OFF = 0x320;

var ms_ticks: usize = undefined;

pub fn calibrate() void {
    timers.pit.start(10);
    lapic.write_reg(INITIAL_COUNT_OFF, 0);
    lapic.write_reg(CONFIG_OFF, 1);

    lapic.write_reg(INITIAL_COUNT_OFF, 0xFFFFFFFF);
    timers.sleep(10);
    ms_ticks = (0xFFFFFFFF - lapic.read_reg(CUR_COUNT_OFF)) / 100;
    stop();
    timers.pit.stop();
    idt.set_ent(0x20, idt.create_irq(0, "lapic_timer_handler"));
    log.debug("LAPIC ticks per millisecond: {}", .{ms_ticks});
}

pub fn start(ms: usize) void {
    lapic.write_reg(LVT_OFF, 0x20 | 0x20000);
    lapic.write_reg(INITIAL_COUNT_OFF, ms * ms_ticks);
    lapic.write_reg(CONFIG_OFF, 1);
}

pub fn stop() void {
    lapic.write_reg(INITIAL_COUNT_OFF, 0);
}

export fn lapic_timer_handler(status: *const idt.Status, _: usize) *const idt.Status {
    _ = @atomicRmw(usize, &timers.ticks, .Add, 1, .acq_rel);
    lapic.write_reg(EOI_OFF, 0);
    return status;
}
