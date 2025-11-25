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

pub var read_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13) u32 = undefined;
pub var write_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13, val: u32) void = undefined;

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

fn find_cap(addr: uacpi.uacpi_pci_address, cap_id: u8) ?u8 {
    const status: u16 = @intCast(read_reg(addr, 4) >> 16);
    // No capability list
    if ((status & (1 << 4)) == 0) return null;

    var ptr: u8 = @truncate(read_reg(addr, 0x34));
    while (ptr != 0) : (ptr = @truncate(read_reg(addr, ptr) >> 8)) {
        const id: u8 = @truncate(read_reg(addr, @intCast(ptr)));
        if (id == cap_id) return ptr;
    }
    return null;
}
