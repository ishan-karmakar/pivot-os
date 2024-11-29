pub const pit = @import("pit.zig");
pub const lapic = @import("lapic.zig");
pub const hpet = @import("hpet.zig");
const cpu = @import("kernel").drivers.cpu;

pub var ticks: usize = 0;

pub inline fn time() usize {
    return @atomicLoad(usize, &ticks, .unordered);
}

/// Initializes global time source
/// If TSC is available, then TSC
/// Otherwise, HPET
pub fn init_source() void {
    hpet.init();
}
