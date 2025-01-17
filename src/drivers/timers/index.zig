pub const pit = @import("pit.zig");
pub const acpi = @import("acpi.zig");
pub const hpet = @import("hpet.zig");
pub const tsc = @import("tsc.zig");
pub const lapic = @import("lapic.zig");
const kernel = @import("kernel");
const cpu = kernel.drivers.cpu;
const std = @import("std");

pub const CallbackFn = *const fn (?*anyopaque, *const cpu.Status) *const cpu.Status;
pub const VTable = struct {
    callback: ?*const fn (ns: usize, ctx: ?*anyopaque, handler: CallbackFn) void,
    time: ?*const fn () usize,
};

pub var NoCalibrationTask = kernel.Task{
    .name = "No Calibration Timers",
    .init = no_cal_init,
    .dependencies = &.{
        .{ .task = &hpet.Task, .accept_failure = true },
        .{ .task = &acpi.Task, .accept_failure = true },
        .{ .task = &pit.Task, .accept_failure = true },
    },
};

var CalibrationTask = kernel.Task{
    .name = "Calibration Timers",
    .init = null,
    .dependencies = &.{
        .{ .task = &tsc.Task, .accept_failure = true },
        .{ .task = &lapic.Task, .accept_failure = true },
    },
};

pub var Task = kernel.Task{
    .name = "Global Time + Timers",
    .init = null,
    .dependencies = &.{
        .{ .task = &NoCalibrationTask },
        .{ .task = &CalibrationTask },
    },
};

pub var gts: ?*const VTable = null;
pub var timer: ?*const VTable = null;
var no_cal_timer: ?*const VTable = null;
var no_cal_gts: ?*const VTable = null;

// TODO: RTC timer

fn no_cal_init() kernel.Task.Ret {
    no_cal_gts = gts;
    no_cal_timer = timer;
    if (gts == null and timer == null) return .failed;
    timer = null;
    gts = null;
    return .success;
}

pub fn sleep(ns: usize) void {
    const _gts = gts orelse no_cal_gts;
    const _timer = timer orelse no_cal_timer;
    if (_gts) |vtable| {
        const t = vtable.time.?;
        const start = t();
        while (t() < (start + ns)) asm volatile ("pause");
    } else if (_timer) |vtable| {
        var triggered = false;
        vtable.callback.?(ns, &triggered, sleep_callback);
        while (!@atomicLoad(bool, @as(*const volatile bool, &triggered), .unordered)) asm volatile ("pause");
    } else @panic("No timer found");
}

fn sleep_callback(ctx: ?*anyopaque, status: *const kernel.drivers.cpu.Status) *const kernel.drivers.cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    return status;
}
