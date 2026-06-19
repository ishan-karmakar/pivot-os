const kernel = @import("root");
const cpu = kernel.drivers.cpu;
const std = @import("std");

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

pub var Task = kernel.Task{
    .name = "Timers",
    .init = init,
    .dependencies = &.{},
};

var clocksource: ?*const ClockSource = null;
var clockevent: ?*const ClockEvent = null;

fn init() kernel.Task.Ret {
    @import("pit.zig").Task.run();
    @import("acpi.zig").Task.run();
    // @import("hpet.zig").Task.run();
    // @import("lapic.zig").Task.run();
    // @import("tsc.zig").Task.run();
    return .success;
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
