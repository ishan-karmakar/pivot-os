const kernel = @import("kernel");
const log = @import("std").log.scoped(.ahci);
const pci = kernel.drivers.pci;

pub const vtable = pci.VTable{
    .target_codes = &.{
        .{ .class_code = 0x1, .subclass_code = 0x6, .prog_if = 0x1 },
    },
    .init = init,
};

fn init(segment: u16, bus: u8, device: u5, func: u3) void {
    log.info("AHCI controller detected", .{});
    log.info("BAR: 0x{x}", .{pci.read_reg(segment, bus, device, func, 0x24)});
}
