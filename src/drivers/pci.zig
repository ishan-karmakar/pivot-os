const kernel = @import("kernel");
const uacpi = @import("uacpi");
const std = @import("std");
const log = std.log.scoped(.pci);
const acpi = kernel.drivers.acpi;
const serial = kernel.drivers.serial;

const CONFIG_ADDR = 0xCF8;
const CONFIG_DATA = 0xCFC;
const PCIE_CONFIG_SPACE_PAGES: comptime_int = ((255 << 20) | (31 << 15) | (7 << 12) + 0x1000) / 0x1000;

pub const Codes = struct {
    class_code: ?u8 = null,
    subclass_code: ?u8 = null,
    prog_if: ?u8 = null,

    fn matches(self: @This(), cc: u8, sc: u8, prog_if: u8) bool {
        if (self.class_code == null) return true;
        if (self.class_code.? != cc) return false;
        if (self.subclass_code == null) return true;
        if (self.subclass_code.? != sc) return true;
        if (self.prog_if == null) return true;
        if (self.prog_if.? != prog_if) return false;
        return true;
    }
};

pub const VTable = struct {
    target_codes: []const Codes,
    target_ret: struct {
        class_code: u8,
        subclass_code: u8,
        prog_if: u8,
        addr: uacpi.uacpi_pci_address,
    },
};

// This should be modified to add or remove PCI drivers
const AVAILABLE_DRIVERS = [_]type{
    kernel.drivers.ide,
};

pub var Task = kernel.Task{
    .name = "PCI(e)",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KMapperTask },
        .{ .task = &kernel.drivers.acpi.TablesTask },
    },
};

pub var read_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13) u32 = undefined;
pub var write_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13, val: u32) void = undefined;
var segment_groups: []const uacpi.acpi_mcfg_allocation = undefined;

fn init() kernel.Task.Ret {
    const pcie_table = acpi.get_table(uacpi.acpi_mcfg, uacpi.ACPI_MCFG_SIGNATURE);
    if (pcie_table) |tbl| {
        init_pcie(tbl);
    } else init_legacy();
    return .success;
}

fn init_legacy() void {
    read_reg = pci_read_reg;
    write_reg = pci_write_reg;

    scan_devices(0);
}

fn scan_devices(segment: u16) void {
    var addr = uacpi.uacpi_pci_address{
        .segment = segment,
        .bus = 0,
        .device = 0,
        .function = 0,
    };
    const header_type = (read_reg(addr, 0xC) >> 16) & 0xFF;
    if (header_type & (1 << 7) == 0) {
        // Single PCI host controller
        addr.bus = 0;
        check_bus(addr);
    } else {
        for (0..8) |f| {
            addr.bus = @intCast(f);
            if (read_reg(addr, 0) == 0xFFFFFFFF) break;
            check_bus(addr);
        }
    }
}

inline fn check_bus(_addr: uacpi.uacpi_pci_address) void {
    var addr = _addr;
    for (0..32) |d| {
        addr.device = @intCast(d);
        check_device(addr);
    }
}

fn check_device(_addr: uacpi.uacpi_pci_address) void {
    var addr = _addr;
    addr.function = 0;
    if (!check_function(addr)) return;
    const header_type = (read_reg(addr, 0xC) >> 16) & 0xFF;
    if (header_type & (1 << 7) > 0) {
        for (1..8) |func| {
            addr.function = @intCast(func);
            _ = check_function(addr);
        }
    }
}

fn check_function(addr: uacpi.uacpi_pci_address) bool {
    const vendor_dev = read_reg(addr, 0);
    if (vendor_dev == 0xFFFFFFFF) return false;
    const codes_raw = read_reg(addr, 0x8);
    const class_code: u8 = @truncate(codes_raw >> 24);
    const subclass_code: u8 = @truncate(codes_raw >> 16);
    const prog_if: u8 = @truncate(codes_raw >> 8);
    log.info(
        "Found function at address {} (VID: 0x{x}, DID: 0x{x}, CC: 0x{x}, SC: 0x{x}, PIF: 0x{x})",
        .{
            addr,
            vendor_dev & 0xFFFF,
            (vendor_dev >> 16) & 0xFFFF,
            class_code,
            subclass_code,
            prog_if,
        },
    );
    inline for (AVAILABLE_DRIVERS) |driver| {
        for (driver.PCIVTable.target_codes) |code| {
            if (code.matches(class_code, subclass_code, prog_if)) {
                driver.PCIVTable.target_ret = .{
                    .class_code = class_code,
                    .subclass_code = subclass_code,
                    .prog_if = prog_if,
                    .addr = addr,
                };
                driver.PCITask.run();
            }
        }
    }
    return true;
}

fn init_pcie(tbl: *const uacpi.acpi_mcfg) void {
    const num_entries = (tbl.hdr.length - @sizeOf(uacpi.acpi_mcfg)) / @sizeOf(uacpi.acpi_mcfg_allocation);
    const groups: [*]const uacpi.acpi_mcfg_allocation = tbl.entries();
    segment_groups = groups[0..num_entries];

    read_reg = pcie_read_reg;
    write_reg = pcie_write_reg;

    for (segment_groups) |seg| {
        for (0..PCIE_CONFIG_SPACE_PAGES) |i| {
            kernel.lib.mem.kmapper.map(seg.address + i * 0x1000, seg.address + i * 0x1000, (1 << 63) | 0b11);
        }
        scan_devices(seg.segment);
    }
}

fn pci_get_addr(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);

    return ((bus << 16) | (device << 11) | (func << 8) | (1 << 31)) + off;
}

fn pci_read_reg(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    serial.out(CONFIG_ADDR, pci_get_addr(addr, off));
    return serial.in(CONFIG_DATA, u32);
}

fn pci_write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: u32) void {
    serial.out(CONFIG_ADDR, pci_get_addr(addr, off));
    serial.out(CONFIG_DATA, val);
}

fn pcie_get_addr(base: usize, addr: uacpi.uacpi_pci_address, off: u13) *u32 {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);
    return @ptrFromInt(base + ((bus << 20) | (device << 15) | (func << 12)) + off);
}

fn pcie_read_reg(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    for (segment_groups) |seg| {
        if (seg.segment == addr.segment) return pcie_get_addr(seg.address, addr, off).*;
    }
    @panic("PCIe segment not found");
}

fn pcie_write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: u32) void {
    for (segment_groups) |seg| {
        if (seg.segment == addr.segment) pcie_get_addr(seg.address, addr, off).* = val;
    }
    @panic("PCIe segment not found");
}
