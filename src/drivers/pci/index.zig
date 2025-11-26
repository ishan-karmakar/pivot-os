const pci = @import("pci.zig");
const pcie = @import("pcie.zig");
const kernel = @import("root");
const uacpi = @import("uacpi");
const std = @import("std");
const log = std.log.scoped(.pci);
const acpi = kernel.drivers.acpi;

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

const AVAILABLE_DRIVERS = [_]type{};

pub var Task = kernel.Task{
    .name = "PCI(e)",
    .init = init,
    .dependencies = &.{
        .{ .task = &pcie.Task, .accept_failure = true },
    },
};

pub fn ReadFunc(T: type) type {
    return *const fn (addr: uacpi.uacpi_pci_address, off: u13) T;
}

pub fn WriteFunc(T: type) type {
    return *const fn (addr: uacpi.uacpi_pci_address, off: u13, val: T) void;
}

const BAR = union(enum) {
    IO: u32,
    MEM: struct {
        addr: usize,
        prefetchable: bool,
    },
};

pub var read_reg8: ReadFunc(u8) = undefined;
pub var read_reg16: ReadFunc(u16) = undefined;
pub var read_reg32: ReadFunc(u32) = undefined;
pub var write_reg8: WriteFunc(u8) = undefined;
pub var write_reg16: WriteFunc(u16) = undefined;
pub var write_reg32: WriteFunc(u32) = undefined;

fn init() kernel.Task.Ret {
    if (pcie.Task.ret == .success) return .success;
    log.warn("PCIe failed to initialize, falling back to legacy PCI", .{});
    pci.Task.run();
    return pci.Task.ret.?;
}

pub fn scan_devices(segment: u16) void {
    var addr = uacpi.uacpi_pci_address{
        .segment = segment,
        .bus = 0,
        .device = 0,
        .function = 0,
    };
    const header_type = read_reg8(addr, 0xC + 2);
    if (header_type & (1 << 7) == 0) {
        // Single PCI host controller
        addr.bus = 0;
        check_bus(addr);
    } else {
        for (0..8) |f| {
            addr.bus = @intCast(f);
            if (read_reg32(addr, 0) == 0xFFFFFFFF) break;
            check_bus(addr);
        }
    }
}

fn check_bus(_addr: uacpi.uacpi_pci_address) void {
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
    const header_type = read_reg8(addr, 0xC + 2);
    if (header_type & (1 << 7) > 0) {
        for (1..8) |func| {
            addr.function = @intCast(func);
            _ = check_function(addr);
        }
    }
}

fn check_function(addr: uacpi.uacpi_pci_address) bool {
    const vendor_dev = read_reg32(addr, 0);
    if (vendor_dev == 0xFFFFFFFF) return false;
    const prog_if = read_reg8(addr, 0x8 + 1);
    const subclass_code = read_reg8(addr, 0x8 + 2);
    const class_code = read_reg8(addr, 0x8 + 3);
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

fn find_cap(addr: uacpi.uacpi_pci_address, cap_id: u8) ?u8 {
    const status: u16 = read_reg16(addr, 4 + 2);
    // No capability list
    if ((status & (1 << 4)) == 0) return null;

    var ptr = read_reg8(addr, 0x34);
    while (ptr != 0) : (ptr = read_reg8(addr, ptr + 1)) {
        const id = read_reg8(addr, @intCast(ptr));
        if (id == cap_id) return ptr;
    }
    return null;
}

pub fn find_bar(addr: uacpi.uacpi_pci_address, idx: u8) BAR {
    const raw = read_reg32(addr, 0x10 + idx * 4);
    if (raw & 1 == 1)
        return .{ .IO = raw & ~@as(u32, 0b11) };

    var bar_addr: usize = @intCast(raw);
    if ((raw >> 1) & 0b11 > 0)
        bar_addr |= @as(u64, read_reg32(addr, 0x10 + (idx + 1) * 4)) << 32;
    return .{ .MEM = .{
        .addr = bar_addr,
        .prefetchable = (raw >> 3) & 1 > 0,
    } };
}
