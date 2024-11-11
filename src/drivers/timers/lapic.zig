const lapic = @import("kernel").drivers.lapic;
const timers = @import("kernel").drivers.timers;
const log = @import("std").log.scoped(.lapic);
const idt = @import("kernel").drivers.idt;

pub var ms_ticks: usize = undefined;
var triggered: bool = false;

pub fn calibrate() void {
    lapic.write_reg(lapic.INITIAL_COUNT_OFF, 0xFFFFFFFF);
    timers.pit.sleep(50);
    ms_ticks = (0xFFFFFFFF - lapic.read_reg(lapic.CUR_COUNT_OFF)) / 50;
    lapic.write_reg(lapic.INITIAL_COUNT_OFF, 0);
    log.debug("LAPIC ticks per millisecond: {}", .{ms_ticks});
}
