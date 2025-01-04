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

pub var GlobalTimeTask = kernel.Task{
    .name = "Global Time",
    .init = null,
    .dependencies = &.{
        .{ .task = &tsc.Task, .accept_failure = true },
        .{ .task = &hpet.GlobalTimeTask, .accept_failure = true },
    },
};

pub var TimerTask = kernel.Task{
    .name = "Timers",
    .init = timers_init,
    .dependencies = &.{
        .{ .task = &hpet.TimerTask, .accept_failure = true },
        .{ .task = &pit.Task, .accept_failure = true },
    },
};

pub var gts: ?*const VTable = null;
pub var timer: ?*const VTable = null;

fn timers_init() kernel.Task.Ret {
    if (timer == null) return .failed;
    // FIXME: If no GTS, use timer instead
    return .success;
}

pub fn sleep(ns: usize) void {
    // const t = gts orelse @panic("No global time source available");
    // const time = t.time orelse @panic("GTS doesn't support time()");
    // const start = time();
    // while (time() < (start + ns)) asm volatile ("pause");
    if (gts) |vtable| {
        const t = vtable.time.?;
        const start = t();
        while (t() < (start + ns)) asm volatile ("pause");
    } else if (timer) |vtable| {
        var triggered = false;
        vtable.callback.?(ns, &triggered, sleep_callback);
        while (!@atomicLoad(bool, @as(*const volatile bool, &triggered), .unordered)) asm volatile ("pause");
    }
}

fn sleep_callback(ctx: ?*anyopaque, status: *const kernel.drivers.cpu.Status) *const kernel.drivers.cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    return status;
}
