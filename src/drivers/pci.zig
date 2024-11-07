const log = @import("std").log.scoped(.pci);
const serial = @import("kernel").drivers.serial;
const uacpi = @import("uacpi");

const CONFIG_ADDRESS = 0xCF8;
const CONFIG_DATA = 0xCFC;

var express: ?usize = null;

pub fn init() void {
    var out_table: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature("MCFG", &out_table) != uacpi.UACPI_STATUS_OK) {
        log.info("MCFG table does not exist, PCIe is not supported, falling back to PCI", .{});
        return;
    }
    // TODO: all of this below is untested
    const mcfg: *const uacpi.acpi_mcfg = @ptrCast(out_table.unnamed_0.hdr);
    var i: usize = 0;
    while (true) {
        const ent: uacpi.acpi_mcfg_allocation = mcfg.entries()[i];
        log.info("MCFG Entry: {x}, {}, {}", .{ ent.address, ent.start_bus, ent.end_bus });
        i += 1;
    }
    @panic("pcie init unimplemented");
}

pub fn read(_bus: u8, _dev: u8, _func: u8, _off: u8, T: type) T {
    const bus: u32 = @intCast(_bus);
    const dev: u32 = @intCast(_dev);
    const func: u32 = @intCast(_func);
    const off: u32 = @intCast(_off);
    serial.out(CONFIG_ADDRESS, (bus << 16) | (dev << 11) | (func << 8) | (off & 0xFC) | 0x80000000);
    return serial.in(CONFIG_DATA, T);
}

pub fn write(_bus: u8, _dev: u8, _func: u8, _off: u8, val: anytype) void {
    const bus: u32 = @intCast(_bus);
    const dev: u32 = @intCast(_dev);
    const func: u32 = @intCast(_func);
    const off: u32 = @intCast(_off);
    serial.out(CONFIG_ADDRESS, (bus << 16) | (dev << 11) | (func << 8) | (off & 0xFC) | 0x80000000);
    serial.out(CONFIG_DATA + (off & 3), val);
}
