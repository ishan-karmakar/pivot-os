const kernel = @import("root");
const cpu = kernel.drivers.cpu;
const log = @import("std").log.scoped(.timers);

pub const CallbackFn = *const fn (?*anyopaque, *cpu.Status) *const cpu.Status;

pub const ClockSource = struct {
    rating: u16,

    read: *const fn () usize,
};

pub const ClockEvent = struct {
    pub const Features = struct {
        per_cpu: bool,
    };
    rating: u16,
    features: Features,
    oneshot: *const fn (ns: u64, ctx: ?*anyopaque, cb: CallbackFn) void,
};

var clocksource: ?*const ClockSource = null;
var clockevent: ?*const ClockEvent = null;

pub fn init() void {
    @import("pit.zig").init() catch {};
    @import("acpi.zig").init() catch {};
    @import("hpet.zig").init() catch {};
    // @import("lapic.zig").Task.run();
    // @import("tsc.zig").Task.run();
    kernel.lib.logger.successfully_initialized(log, "Timer Subsystem");
}

/// Registers a new clockevent. The actual clockevent that is chosen depends on the rating comparison
pub fn register_clockevent(ce: *const ClockEvent) void {
    if (clockevent == null or ce.rating > clockevent.?.rating)
        clockevent = ce;
}

/// Registers a new clocksource. The actual clocksource that is chosen depends on the rating comparison
pub fn register_clocksource(cs: *const ClockSource) void {
    if (clocksource == null or cs.rating > clocksource.?.rating)
        clocksource = cs;
}

pub fn sleep(ns: usize) void {
    const start = time();
    while (time() < (start + ns)) asm volatile ("pause");
}

pub fn time() usize {
    return (clocksource orelse @panic("No timer available")).read();
}
