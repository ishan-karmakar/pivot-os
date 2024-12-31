pub const pit = @import("pit.zig");
// pub const lapic = @import("lapic.zig");
pub const acpi = @import("acpi.zig");
pub const hpet = @import("hpet.zig");
pub const tsc = @import("tsc.zig");
const kernel = @import("kernel");
const std = @import("std");

pub const VTable = struct {
    /// nanoseconds between each tick
    min_interval: f64,
    /// Maximum nanoseconds timer can natively sleep
    max_interval: usize,
    init: *const fn () bool,
    time: ?*const fn () usize,
    sleep: *const fn (ns: usize) bool,
};

const TIMERS = [_]*const VTable{
    &pit.vtable,
    &hpet.vtable,
    &tsc.vtable,
};

var usable_timers: std.ArrayList(*const VTable) = undefined;

pub var ticks: usize = 0;
var gtime_source: ?*const VTable = null;
var gtimer: ?*const VTable = null;

pub fn init() void {
    usable_timers = std.ArrayList(*const VTable).init(kernel.lib.mem.kheap.allocator());

    for (TIMERS) |timer| {
        if (timer.init()) usable_timers.append(timer) catch @panic("OOM");
    }

    // std.mem.sort(*const VTable, usable_timers.items, {}, timer_comparator);
}

fn timer_comparator(_: void, lhs: *const VTable, rhs: *const VTable) bool {
    return (lhs.max_interval - lhs.min_interval) < (rhs.max_interval - rhs.max_interval);
}
