pub const pit = @import("pit.zig");
pub const acpi = @import("acpi.zig");
pub const hpet = @import("hpet.zig");
pub const tsc = @import("tsc.zig");
pub const lapic = @import("lapic.zig");
const kernel = @import("kernel");
const cpu = kernel.drivers.cpu;
const std = @import("std");

pub const CallbackFn = *const fn (?*anyopaque, *cpu.Status) *const cpu.Status;
pub const TimerVTable = struct {
    requires_calibration: bool,
    callback: *const fn (ns: usize, ctx: ?*anyopaque, handler: CallbackFn) void,
};
pub const GTSVTable = struct {
    requires_calibration: bool,
    time: *const fn () usize,
};

pub var Task = kernel.Task{
    .name = "Timer + GTS",
    .init = init,
    .dependencies = &.{},
};

const AVAILABLE_TIMERS = [_]type{
    lapic,
    hpet,
    pit,
};

const AVAILABLE_GTS = [_]type{
    tsc,
    hpet,
    acpi,
};

pub var gts: ?*const GTSVTable = null;
pub var timer: ?*const TimerVTable = null;

fn init() kernel.Task.Ret {
    inline for (AVAILABLE_TIMERS) |src| {
        if (src.TimerVTable.requires_calibration) {
            init_no_cal_gts();
            if (gts == null) return .failed;
        }
        src.TimerTask.run();
        if (src.TimerTask.ret == .success) {
            timer = &src.TimerVTable;
            break;
        }
    }

    inline for (AVAILABLE_GTS) |src| {
        if (src.GTSVTable.requires_calibration) {
            init_no_cal_gts();
            // This would mean that all no calibration GTS failed, meaning calibrated GTSs can't run either
            if (gts == null) return .failed;
        }
        src.GTSTask.run();
        if (src.GTSTask.ret == .success) {
            gts = &src.GTSVTable;
            break;
        }
    }

    // Here if there is no GTS then we cannot keep track of absolute time since boot
    // TODO: Backup for no GTS, periodic IRQ every 10 ms?
    if (gts == null or timer == null) return .failed;
    return .success;
}

fn init_no_cal_timer() void {
    inline for (AVAILABLE_TIMERS) |src| {
        if (!src.TimerVTable.requires_calibration) {
            src.TimerTask.run();
            if (src.TimerTask.ret == .success) {
                timer = &src.TimerVTable;
                break;
            }
        }
    }
}

fn init_no_cal_gts() void {
    inline for (AVAILABLE_GTS) |src| {
        if (!src.GTSVTable.requires_calibration) {
            src.GTSTask.run();
            if (src.GTSTask.ret == .success) {
                gts = &src.GTSVTable;
                break;
            }
        }
    }
    if (gts == null) init_no_cal_timer();
}

pub fn time() usize {
    return (gts orelse @panic("No global time source available")).time();
}

pub fn sleep(ns: usize) void {
    if (gts) |vtable| {
        const start = vtable.time();
        while (vtable.time() < (start + ns)) asm volatile ("pause");
    } else if (timer) |vtable| {
        // TODO: Consider std.atomic.Value?
        var triggered = false;
        vtable.callback(ns, &triggered, sleep_callback);
        while (!@atomicLoad(bool, @as(*const volatile bool, &triggered), .unordered)) asm volatile ("pause");
    } else @panic("No timer found");
}

pub fn callback(ns: usize, ctx: ?*anyopaque, func: CallbackFn) void {
    timer.callback(ns, ctx, func);
}

fn sleep_callback(ctx: ?*anyopaque, status: *const kernel.drivers.cpu.Status) *const kernel.drivers.cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    return status;
}
