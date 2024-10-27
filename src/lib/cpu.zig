const gdt = @import("../drivers/gdt.zig");
const idt = @import("../drivers/idt.zig");

pub fn init() void {
    gdt.init_static();
    idt.init();
}
