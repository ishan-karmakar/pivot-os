const limine = @import("limine");
const pmm = @import("kernel").lib.mem.pmm;

pub fn init() void {
    pmm.init();
}
