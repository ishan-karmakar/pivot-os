const lapic = @import("kernel").drivers.lapic;
const timers = @import("kernel").drivers.timers;
const log = @import("std").log.scoped(.lapic);
const idt = @import("kernel").drivers.idt;

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;

var ms_ticks: usize = undefined;
var triggered: bool = false;

pub fn calibrate() void {
    lapic.write_reg(INITIAL_COUNT_OFF, 0);
    lapic.write_reg(CONFIG_OFF, 1);

    lapic.write_reg(INITIAL_COUNT_OFF, 0xFFFFFFFF);
    timers.pit.sleep(50);
    ms_ticks = (0xFFFFFFFF - lapic.read_reg(CUR_COUNT_OFF)) / 50;
    stop();
    log.debug("LAPIC ticks per millisecond: {}", .{ms_ticks});
}

pub fn sleep(ms: usize) void {
    lapic.write_reg(INITIAL_COUNT_OFF, ms * ms_ticks);
    while (@cmpxchgWeak(bool, &triggered, true, false, .acq_rel, .monotonic) != null) {}
    stop();
}

fn stop() void {
    lapic.write_reg(INITIAL_COUNT_OFF, 0);
}