const kernel = @import("kernel");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;
const log = @import("std").log.scoped(.lapic);
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const scheduler = kernel.lib.scheduler;
const smp = kernel.drivers.smp;

pub var ms_ticks: usize = undefined;
pub var interval: usize = undefined;
pub const TIMER_VEC = 0x20;

pub fn calibrate() void {
    lapic.write_reg(lapic.INITIAL_COUNT_OFF, 0xFFFFFFFF);
    timers.pit.sleep(50);
    ms_ticks = (0xFFFFFFFF - lapic.read_reg(lapic.CUR_COUNT_OFF)) / 50;
    lapic.write_reg(lapic.INITIAL_COUNT_OFF, 0);
    log.debug("LAPIC ticks per millisecond: {}", .{ms_ticks});
    idt.set_ent(TIMER_VEC, idt.create_irq(TIMER_VEC, "lapic_timer_handler"));
}

pub fn start(_interval: usize) void {
    interval = _interval;
    lapic.write_reg(lapic.INITIAL_COUNT_OFF, ms_ticks * interval);
}

export fn lapic_timer_handler(status: *const cpu.Status, vec: usize) *const cpu.Status {
    _ = @atomicRmw(usize, &timers.ticks, .Add, interval, .monotonic);
    lapic.eoi();
    return scheduler.schedule(status, vec);
}
