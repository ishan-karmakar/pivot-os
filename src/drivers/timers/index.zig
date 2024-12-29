pub const pit = @import("pit.zig");
// pub const lapic = @import("lapic.zig");
pub const acpi = @import("acpi.zig");
pub const hpet = @import("hpet.zig");
pub const tsc = @import("tsc.zig");
const cpu = @import("kernel").drivers.cpu;

pub const CAPABILITIES_IRQ = 0b1;
pub const CAPABILITIES_COUNTER = 0b10;
pub const VTable = struct {
    init: *const fn () bool,
    time: ?*const fn () usize,
    sleep: *const fn (ns: usize) void, // All timers should be able to sleep
    // set_oneshot: ?*const fn (ns: usize, callback: *const fn () void, ctx: ?*anyopaque) void,
};

pub var ticks: usize = 0;
var gtime_source: ?*const VTable = null;
var gtimer: ?*const VTable = null;

pub inline fn time() usize {
    return 0;
    // return @atomicLoad(usize, &ticks, .unordered);
}

pub fn init() void {
    // Two categories: global time source and actual timer (with IRQ on threshold)
    // Timer: LAPIC -> HPET -> PIT
    // Global time sources: TSC -> HPET -> ACPI -> Any timer with periodic IRQs
    init_gtime_source();
    init_gtimer();
}

fn init_gtime_source() void {
    _ = acpi.vtable.init();
    // if (tsc.vtable.init()) {
    //     gtime_source = &tsc.vtable;
    // } else if (hpet.vtable.init()) {
    //     gtime_source = &hpet.vtable;
    // }
}

fn init_gtimer() void {}
