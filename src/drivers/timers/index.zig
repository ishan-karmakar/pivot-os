pub const pit = @import("pit.zig");
pub const acpi = @import("acpi.zig");
pub const hpet = @import("hpet.zig");
pub const tsc = @import("tsc.zig");
pub const lapic = @import("lapic.zig");
const kernel = @import("kernel");
const cpu = kernel.drivers.cpu;
const std = @import("std");

pub const GTSVTable = struct {
    time: *const fn () usize,
    deinit: *const fn () void,
};

pub const CallbackFn = *const fn (?*anyopaque, *const cpu.Status) *const cpu.Status;
pub const TimerVTable = struct {
    callback: *const fn (ns: usize, ctx: ?*anyopaque, handler: CallbackFn) void,
    deinit: *const fn () void,
};

pub var Task = kernel.Task{
    .name = "Timers",
    .init = null,
    .dependencies = &.{
        .{ .task = &pit.Task, .accept_failure = true },
        .{ .task = &hpet.Task, .accept_failure = true },
        .{ .task = &lapic.Task, .accept_failure = true },
        // .{ .task = &tsc.Task, .accept_failure = true },
    },
};

var gts: ?*const GTSVTable = null;
var timer: ?*const TimerVTable = null;

pub fn sleep(ns: usize) void {
    // FIXME: Either GTS or timer for sleep
    const t = gts orelse @panic("No global time source available");
    const start = t.time();
    while (t.time() < (start + ns)) asm volatile ("pause");
}

pub fn set_timer(t: *const TimerVTable) void {
    if (timer) |org| org.deinit();
    timer = t;
}

pub fn set_gts(t: *const GTSVTable) void {
    if (gts) |org| org.deinit();
    gts = t;
}
