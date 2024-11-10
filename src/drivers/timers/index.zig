pub const pit = @import("pit.zig");
pub const lapic = @import("lapic.zig");

/// This is an ESTIMATE of the current time
/// In reality, it is only updated once every ~50 ms
pub inline fn time() usize {
    return 0;
}
