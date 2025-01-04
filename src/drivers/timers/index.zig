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
    callback: ?*const fn (self: *@This(), ns: usize, ctx: ?*anyopaque, handler: CallbackFn) void,
    time: ?*const fn (self: *@This()) usize,
};

pub var Task = kernel.Task{
    .name = "Timers",
    .init = init,
    .dependencies = &.{
        // .{ .task = &tsc.Task, .accept_failure = true },
        // .{ .task = &lapic.Task, .accept_failure = true },
        .{ .task = &hpet.Task, .accept_failure = true },
        .{ .task = &pit.Task, .accept_failure = true },
    },
};

pub var gts: ?*const VTable = null;
pub var timer: ?*const VTable = null;

fn init() kernel.Task.Ret {
    if (timer == null) return .failed;
    // FIXME: If no GTS, use timer instead
    return .success;
}

pub fn sleep(ns: usize) void {
    // FIXME: Either GTS or timer for sleep
    const t = gts orelse @panic("No global time source available");
    const time = t.time orelse @panic("GTS doesn't support time()");
    const start = time();
    while (time() < (start + ns)) asm volatile ("pause");
}
