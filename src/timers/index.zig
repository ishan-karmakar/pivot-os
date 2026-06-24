const kernel = @import("root");
const cpu = kernel.cpu;
const std = @import("std");
const log = std.log.scoped(.timers);

pub const CallbackFn = *const fn (?*anyopaque) void;

pub const ClockSource = struct {
    rating: u16,

    read: *const fn () usize,
};

pub const ClockEvent = struct {
    rating: u16,
    oneshot: *const fn (ns: u64, ctx: ?*anyopaque, cb: CallbackFn) void,
};

const Wakeup = struct {
    wake_time: usize,
    ctx: ?*anyopaque,
    func: CallbackFn,
};

var clocksource: ?*const ClockSource = null;
var clockevent: ?*const ClockEvent = null;

const wakeups: std.PriorityQueue(Wakeup, void, compare_wakeups) = .empty;

pub fn init() !void {
    @import("pit.zig").init() catch {};
    @import("acpi.zig").init() catch {};
    @import("hpet.zig").init() catch {};
    @import("lapic.zig").init() catch {};
    @import("tsc.zig").init() catch {};

    if (clocksource == null)
        return kernel.lib.logger.failed_initialization(log, "Timer Subsystem", error.NoClockSourceFound);
    if (clockevent == null)
        return kernel.lib.logger.failed_initialization(log, "Timer Subsystem", error.NoClockEventFound);
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

pub fn schedule(ns: u64, ctx: ?*anyopaque, cb: CallbackFn) !void {
    try wakeups.push(kernel.mem.kheap.allocator(), .{
        .ctx = ctx,
        .func = cb,
        .wake_time = time() + ns,
    });
    clockevent.?.oneshot(wakeups.peek().?.wake_time - time(), null, timer_interrupt);
}

pub fn sleep(ns: usize) void {
    const start = time();
    while (time() < (start + ns)) asm volatile ("pause");
}

pub fn time() usize {
    return clocksource.?.read();
}

fn timer_interrupt(_: ?*anyopaque) void {
    const now = time();
    while (wakeups.peek()) |wakeup| {
        if (wakeup.wake_time > now) break;

        const expired = wakeups.pop().?;
        expired.func(expired.ctx);
    }
    if (wakeups.peek()) |next_wakeup|
        clockevent.?.oneshot(next_wakeup.wake_time - now, null, timer_interrupt);
}

fn compare_wakeups(_: void, a: Wakeup, b: Wakeup) std.math.Order {
    return std.math.order(a.wake_time, b.wake_time);
}
