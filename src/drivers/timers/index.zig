pub const pit = @import("pit.zig");
pub const lapic = @import("lapic.zig");

pub var ticks: usize = 0;

pub inline fn time() usize {
    return @atomicLoad(usize, &ticks, .unordered);
}
