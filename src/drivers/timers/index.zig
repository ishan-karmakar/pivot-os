pub const hpet = @import("hpet.zig");
pub const acpi = @import("acpi.zig");
pub const pit = @import("pit.zig");
pub const lapic = @import("lapic.zig");
pub const tsc = @import("tsc.zig");
const kernel = @import("root");
const cpu = kernel.drivers.cpu;
const std = @import("std");

pub const CallbackFn = *const fn (?*anyopaque, *cpu.Status) *const cpu.Status;
pub const VTable = struct {
    capabilities: struct {
        // Whether this time source is fixed frequency or dynamic (requires calibration by another timer)
        FIXED: bool,
    },
    // Number of "timers" this specific timer provides (ex. LAPIC provides the number of cores, HPET provides number of comparators)
    num_timers: usize = 1,
    time: ?*const fn () usize,
    callback: ?*const fn (ns: usize, ctx: ?*anyopaque, handler: CallbackFn) void,
};

pub var Task = kernel.Task{
    .name = "Timer + GTS",
    .init = init,
    .dependencies = &.{},
};

// Because the global time source timer is always initialized after the timers, it does not need to be in this init order
const TIMER_INIT_ORDER = [_]type{
    pit,
    // acpi,
    // hpet,
    // lapic,
};

const USAGE_ORDER = [_]type{
    // tsc,
    // lapic,
    // hpet,
    // acpi,
    pit,
};

fn init() kernel.Task.Ret {
    inline for (TIMER_INIT_ORDER) |timer| if (timer.VTable.callback != null) {
        // If timer has the ability to do callbacks, then initialize it
        // Doesn't hurt to initialize all of these timers
        timer.Task.run();
    };
    inline for (USAGE_ORDER) |timer| if (timer.VTable.time != null) {
        // Only initialize one global time source
        timer.Task.run();
        break;
    };

    return .success;
}

pub fn time() usize {
    inline for (USAGE_ORDER) |timer| if (timer.Task.ret.? == .success) if (timer.VTable.time) |t| return t();
    @panic("No absolute time capable time source found");
}

pub fn sleep(ns: usize) void {
    _ = ns;
    @panic("sleep()");
}

pub fn callback(ns: usize, ctx: ?*anyopaque, func: CallbackFn) void {
    inline for (USAGE_ORDER) |timer| if (timer.Task.ret.? == .success) if (timer.VTable.callback) |c| {
        c(ns, ctx, func);
        return;
    };
    @panic("No callback capable time source found");
}
