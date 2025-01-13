const kernel = @import("kernel");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = std.log.scoped(.lapic_timer);

pub var vtable = timers.TimerVTable{
    .callback = null,
    .deinit = deinit,
};

pub var Task = kernel.Task{
    .name = "Local APIC Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &lapic.Task, .accept_failure = true },
        .{ .task = &timers.tsc.Task, .accept_failure = true },
    },
};

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;
const TIMER_OFF = 0x320;

const CALIBRATION_NS = 1_000_000; // 1 ms

var handler: timers.CallbackFn = undefined;
var idt_handler: *idt.HandlerData = undefined;

fn init() kernel.Task.Ret {
    if (lapic.Task.ret != .success) return .failed;

    const tsc_deadline = cpu.cpuid(0x1, 0).ecx & (1 << 24);
    if (tsc_deadline > 0 and timers.tsc.Task.ret == .success) {
        // We are actually able to use TSC Deadline
        log.debug("Using TSC deadine mode", .{});
        vtable.callback = tsc_deadline_callback;
    }

    log.debug("Using normal oneshot mode", .{});
    vtable.callback = oneshot_callback;
    // Calibrate timer

    return .success;
}

fn tsc_deadline_callback(ns: usize, ctx: ?*anyopaque, _handler: timers.CallbackFn) void {
    const ticks = (ns * timers.tsc.hertz) / 1_000_000_000;
}

fn oneshot_callback(ns: usize, ctx: ?*anyopaque, _handler: timers.CallbackFn) void {
    _ = ns;
    _ = ctx;
    _ = _handler;
}

fn deinit() void {}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const ret = handler(ctx, status);
    // LAPIC has no IRQ
    kernel.drivers.intctrl.controller.eoi(0);
    return ret;
}
