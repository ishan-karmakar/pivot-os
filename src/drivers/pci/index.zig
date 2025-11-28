const pci = @import("pci.zig");
const pcie = @import("pcie.zig");
const prt = @import("prt.zig");
const kernel = @import("root");
const uacpi = @import("uacpi");
const std = @import("std");
const log = std.log.scoped(.pci);
const acpi = kernel.drivers.acpi;
const idt = kernel.drivers.idt;

pub const Codes = struct {
    class_code: ?u8 = null,
    subclass_code: ?u8 = null,
    prog_if: ?u8 = null,
    vendor_id: ?u16 = null,
    device_id: ?u16 = null,

    fn matches(self: @This(), info: DeviceInfo) bool {
        return !((self.class_code != null and self.class_code != info.class_code) or
            (self.subclass_code != null and self.subclass_code != info.subclass_code) or
            (self.prog_if != null and self.prog_if != info.prog_if) or
            (self.vendor_id != null and self.vendor_id != info.vendor_id) or
            (self.device_id != null and self.device_id != info.device_id));
    }
};

pub const VTable = struct {
    target_codes: []const Codes,
    target_ret: DeviceInfo,
};

pub var Task = kernel.Task{
    .name = "PCI(e)",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.intctrl.Task },
        .{ .task = &kernel.drivers.acpi.NamespaceLoadTask },
        .{ .task = &pcie.Task, .accept_failure = true },
    },
};

pub fn ReadFunc(T: type) type {
    return *const fn (addr: uacpi.uacpi_pci_address, off: u13) T;
}

pub fn WriteFunc(T: type) type {
    return *const fn (addr: uacpi.uacpi_pci_address, off: u13, val: T) void;
}

pub const DeviceInfo = struct {
    pub const Capability = struct {
        id: u8,
        // Offset into the PCI config space
        offset: u8,
    };

    pub const BAR = union(enum) {
        IO: u32,
        MEM: struct {
            addr: usize,
            prefetchable: bool,
        },
    };

    addr: uacpi.uacpi_pci_address,
    vendor_id: u16,
    device_id: u16,
    class_code: u8,
    subclass_code: u8,
    prog_if: u8,
    bars: []BAR,
    capabilities: []Capability,
    interrupt_pin: ?u8,
    header_type: u8,

    pub fn init(addr: uacpi.uacpi_pci_address) !?@This() {
        if (!is_valid(addr)) return null;
        const interrupt_pin = read_reg8(addr, 0x3C + 1);
        return .{
            .addr = addr,
            .vendor_id = read_reg16(addr, 0),
            .device_id = read_reg16(addr, 2),
            .class_code = read_reg8(addr, 0x8 + 3),
            .subclass_code = read_reg8(addr, 0x8 + 2),
            .prog_if = read_reg8(addr, 0x8 + 1),
            .interrupt_pin = if (interrupt_pin == 0) null else interrupt_pin - 1,
            .header_type = read_reg8(addr, 0xC + 2),
            .bars = try get_bars(addr),
            .capabilities = try get_capabilities(addr),
        };
    }

    pub inline fn is_valid(addr: uacpi.uacpi_pci_address) bool {
        return read_reg16(addr, 0) != 0xFFFF;
    }
};

pub const RoutingType = union(enum) {
    MSI: void,
    MSIX: void,
    PRT: u32,
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

pub fn scan_devices(segment: u16) !void {
    var addr = uacpi.uacpi_pci_address{ .segment = segment };
    const bus_device_info = try DeviceInfo.init(addr) orelse return;
    if (bus_device_info.header_type & (1 << 7) == 0) {
        // Single PCI host controller
        addr.bus = 0;
        try check_bus(addr);
    } else {
        for (0..8) |f| {
            addr.bus = @intCast(f);
            if (!DeviceInfo.is_valid(addr)) break;
            try check_bus(addr);
        }
    }
}

fn check_bus(_addr: uacpi.uacpi_pci_address) !void {
    var addr = _addr;
    for (0..32) |d| {
        addr.device = @intCast(d);
        try check_device(addr);
    }
}

fn check_device(_addr: uacpi.uacpi_pci_address) !void {
    var addr = _addr;
    addr.function = 0;
    const info = try check_function(addr) orelse return;
    if (info.header_type & (1 << 7) > 0) {
        for (1..8) |func| {
            addr.function = @intCast(func);
            _ = try check_function(addr);
        }
    }
}

fn check_function(addr: uacpi.uacpi_pci_address) !?DeviceInfo {
    const info = try DeviceInfo.init(addr) orelse return null;
    log.info(
        "Found function at address {} (VID: 0x{x}, DID: 0x{x}, CC: 0x{x}, SC: 0x{x}, PIF: 0x{x})",
        .{
            addr,
            info.vendor_id,
            info.device_id,
            info.class_code,
            info.subclass_code,
            info.prog_if,
        },
    );
    inline for (kernel.drivers.PCI_DRIVER_LIST) |driver| {
        for (driver.PCIVTable.target_codes) |code| {
            if (code.matches(info)) {
                driver.PCIVTable.target_ret = info;
                driver.PCITask.run();
            }
        }
    }
    return info;
}

pub fn setup_handlers(info: DeviceInfo, handlers: []*idt.HandlerData) RoutingType {
    // Try MSI-X first, then return handler if success
    // Try MSI second, then return handler if success
    if (handlers.len != 1) @panic("Exactly one handler needed for PRT based routing");
    return .{ .PRT = prt.setup_handler(info, handlers[0]) };
}

fn get_bars(addr: uacpi.uacpi_pci_address) ![]DeviceInfo.BAR {
    var bars = std.ArrayList(DeviceInfo.BAR).empty;
    var i: u8 = 0;
    while (i < 5) : (i += 1) {
        const raw = read_reg32(addr, 0x10 + i * 4);
        if (raw & 1 == 1) {
            try bars.append(kernel.lib.mem.kheap.allocator(), .{ .IO = raw & ~@as(u32, 0b11) });
            continue;
        }

        var bar_addr: usize = @intCast(raw);
        if ((raw >> 1) & 0b11 > 0) {
            i += 1;
            bar_addr |= @as(u64, read_reg32(addr, 0x10 + i * 4)) << 32;
        }
        try bars.append(kernel.lib.mem.kheap.allocator(), .{ .MEM = .{
            .addr = bar_addr,
            .prefetchable = (raw >> 3) & 1 > 0,
        } });
    }
    return try bars.toOwnedSlice(kernel.lib.mem.kheap.allocator());
}

fn get_capabilities(addr: uacpi.uacpi_pci_address) ![]DeviceInfo.Capability {
    const status: u16 = read_reg16(addr, 4 + 2);
    // Check if capability list exists
    if ((status & (1 << 4)) == 0) return &.{};

    var capabilities = std.ArrayList(DeviceInfo.Capability).empty;
    var ptr = read_reg8(addr, 0x34);
    while (ptr != 0) : (ptr = read_reg8(addr, ptr + 1)) {
        try capabilities.append(kernel.lib.mem.kheap.allocator(), .{
            .id = read_reg8(addr, @intCast(ptr)),
            .offset = ptr,
        });
    }
    return try capabilities.toOwnedSlice(kernel.lib.mem.kheap.allocator());
}
