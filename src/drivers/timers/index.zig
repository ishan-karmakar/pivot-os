pub const pit = @import("pit.zig");
pub const lapic = @import("lapic.zig");
pub const hpet = @import("hpet.zig");
const cpu = @import("kernel").drivers.cpu;

pub const VTable = struct {
    init: *const fn () bool,
    sleep: *const fn (ns: usize) void,
    set_oneshot: *const fn (ns: usize, callback: *const fn () void) bool,
    set_periodic: *const fn (ns: usize, callback: *const fn () void) bool,
    stop: *const fn () void,
};

pub var ticks: usize = 0;
const gtime_source: ?*const VTable = null;
const gtimer: ?*const VTable = null;

pub inline fn time() usize {
    return @atomicLoad(usize, &ticks, .unordered);
}

pub fn init() void {
    // Two categories: global time source and actual timer (with IRQ on threshold)
    // Timer: LAPIC -> HPET -> PIT
    // Global time sources: TSC -> HPET -> ACPI -> Any timer with periodic IRQs
}
