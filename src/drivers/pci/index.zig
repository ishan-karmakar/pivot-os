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
    init: *const fn (u16, u8, u5, u3) void,
};

const drivers = [_]*const VTable{};

pub var Task = kernel.Task{
    .name = "PCI(e)",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KMapperTask },
        .{ .task = &kernel.drivers.acpi.TablesTask },
    },
};

pub var read_reg: *const fn (segment: u16, bus: u8, device: u5, func: u3, off: u8) u32 = undefined;
pub var write_reg: *const fn (segment: u16, bus: u8, device: u5, func: u3, off: u8, val: u32) void = undefined;
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

    log.info("Initialized legacy PCI", .{});

    scan_devices(0);
}

fn scan_devices(segment: u16) void {
    const header_type = (read_reg(segment, 0, 0, 0, 0xC) >> 16) & 0xFF;
    if (header_type & (1 << 7) == 0) {
        // Single PCI host controller
        check_bus(segment, 0);
    } else {
        for (0..8) |f| {
            if (read_reg(segment, 0, 0, @intCast(f), 0) == 0xFFFFFFFF) break;
            check_bus(segment, @intCast(f));
        }
    }
}

inline fn check_bus(segment: u16, bus: u8) void {
    for (0..32) |d| check_device(segment, bus, @intCast(d));
}

fn check_device(segment: u16, bus: u8, device: u5) void {
    if (!check_function(segment, bus, device, 0)) return;
    const header_type = (read_reg(segment, bus, device, 0, 0xC) >> 16) & 0xFF;
    if (header_type & (1 << 7) > 0) {
        for (1..8) |func| _ = check_function(segment, bus, device, @intCast(func));
    }
}

fn check_function(segment: u16, bus: u8, device: u5, func: u3) bool {
    const vendor_dev = read_reg(segment, bus, device, func, 0);
    if (vendor_dev == 0xFFFFFFFF) return false;
    const codes_raw = read_reg(segment, bus, device, func, 0x8);
    const class_code: u8 = @truncate(codes_raw >> 24);
    const subclass_code: u8 = @truncate(codes_raw >> 16);
    const prog_if: u8 = @truncate(codes_raw >> 8);
    log.info(
        "Found function at S: {}, B: {}, D: {}, F: {} (VID: 0x{x}, DID: 0x{x}, CC: 0x{x}, SC: 0x{x}, PIF: 0x{x})",
        .{
            segment,
            bus,
            device,
            func,
            vendor_dev & 0xFFFF,
            (vendor_dev >> 16) & 0xFFFF,
            class_code,
            subclass_code,
            prog_if,
        },
    );
    for (drivers) |driver| {
        for (driver.target_codes) |code| {
            if (code.matches(class_code, subclass_code, prog_if)) {
                driver.init(segment, bus, device, func);
                break;
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

    log.info("Initialized PCIe", .{});

    for (segment_groups) |seg| {
        for (0..PCIE_CONFIG_SPACE_PAGES) |i| {
            kernel.lib.mem.kmapper.map(seg.address + i * 0x1000, seg.address + i * 0x1000, (1 << 63) | 0b11);
        }
        scan_devices(seg.segment);
    }
}

inline fn get_vendor_id(seg: u16, bus: u8, device: u5, func: u3) u16 {
    return @truncate(read_reg(seg, bus, device, func, 0));
}

fn pci_get_addr(_bus: u8, _device: u5, _func: u3, _off: u8) u32 {
    const bus: u32 = @intCast(_bus);
    const device: u32 = @intCast(_device);
    const func: u32 = @intCast(_func);
    const off: u32 = @intCast(_off);

    return (bus << 16) | (device << 11) | (func << 8) | (off & 0xFC) | (1 << 31);
}

fn pci_read_reg(_: u16, bus: u8, device: u5, func: u3, off: u8) u32 {
    serial.out(CONFIG_ADDR, pci_get_addr(bus, device, func, off));
    return serial.in(CONFIG_DATA, u32);
}

fn pci_write_reg(_: u16, bus: u8, device: u5, func: u3, off: u8, val: u32) void {
    serial.out(CONFIG_ADDR, pci_get_addr(bus, device, func, off));
    serial.out(CONFIG_DATA, val);
}

fn pcie_get_addr(base: usize, _bus: u8, _device: u5, _func: u3, _off: u8) *u32 {
    const bus: u32 = @intCast(_bus);
    const device: u32 = @intCast(_device);
    const func: u32 = @intCast(_func);
    const off: u32 = @intCast(_off);
    return @ptrFromInt(base + ((bus << 20) | (device << 15) | (func << 12)) + (off & 0xFC));
}

fn pcie_read_reg(segment: u16, bus: u8, device: u5, func: u3, off: u8) u32 {
    for (segment_groups) |seg| {
        if (seg.segment == segment) return pcie_get_addr(seg.address, bus, device, func, off).*;
    }
    @panic("PCIe segment not found");
}

fn pcie_write_reg(segment: u16, bus: u8, device: u5, func: u3, off: u8, val: u32) void {
    for (segment_groups) |seg| {
        if (seg.segment == segment) pcie_get_addr(seg.address, bus, device, func, off).* = val;
    }
    @panic("PCIe segment not found");
}
