const limine = @import("limine");
const pmm = @import("../lib/mem/pmm.zig");

pub fn init() void {
    pmm.init();
}
