pub const pit = @import("pit.zig");

pub var ticks: usize = 0;

pub fn sleep(ms: usize) void {
    const org = ticks;
    while (@atomicLoad(usize, &ticks, .acquire) < (org + ms)) {}
}