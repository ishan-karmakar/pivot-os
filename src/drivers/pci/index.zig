const pci = @import("pci.zig");
const pcie = @import("pcie.zig");
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

const PCI_DEVICE_PNP_IDS = [_][*c]const u8{
    "PNP0A03",
    "PNP0A08",
    @ptrCast(uacpi.UACPI_NULL),
};

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
    return .{ .PRT = get_handler_prt(info, handlers[0]) };
}

// Get handler through PCI routing table
fn get_handler_prt(info: DeviceInfo, handler: *idt.HandlerData) u32 {
    var data = .{ info, @as(u32, 0) };
    if (uacpi.uacpi_namespace_for_each_child(
        uacpi.uacpi_namespace_get_predefined(uacpi.UACPI_PREDEFINED_NAMESPACE_SB),
        get_handler_prt_device_callback,
        null,
        uacpi.UACPI_OBJECT_DEVICE_BIT,
        // TODO: Could restrict to depth 1?
        uacpi.UACPI_MAX_DEPTH_ANY,
        &data,
        // &addr,
    ) != uacpi.UACPI_STATUS_OK) @panic("Failed to iterate over PCI devices");
    _ = kernel.drivers.intctrl.map(idt.handler2vec(handler), data.@"1") catch @panic("Failed to map PCI device's GSI");
    return data.@"1";
}

fn get_handler_prt_device_callback(_data: ?*anyopaque, node: ?*uacpi.uacpi_namespace_node, _: uacpi.uacpi_u32) callconv(.c) uacpi.uacpi_iteration_decision {
    const data: *@Tuple(&.{ DeviceInfo, u32 }) = @ptrCast(@alignCast(_data));
    const info = data.@"0";

    // Check if the device is actually a PCI bus
    if (!uacpi.uacpi_device_matches_pnp_id(node, @ptrCast(&PCI_DEVICE_PNP_IDS)))
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Check if the bus number of PCI device is same as actual device (i.e. is device on this PCI bus?)
    var bbn: u64 = 0;
    _ = uacpi.uacpi_eval_simple_integer(node, "_BBN", &bbn);
    if (bbn != info.addr.bus)
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Get the PCI routing table
    var prt: *uacpi.uacpi_pci_routing_table = undefined;
    if (uacpi.uacpi_get_pci_routing_table(node, @ptrCast(&prt)) != uacpi.UACPI_STATUS_OK) {
        log.warn("Failed to get the PCI routing table from PCI device", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Get interrupt pin from the device's config space
    if (info.interrupt_pin == null) {
        log.warn("PCI device does not use an interrupt pin", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Iterate over the routing table entries in search for one that matches our device
    for (0..prt.num_entries) |i| {
        const entry: uacpi.uacpi_pci_routing_table_entry = prt.entries()[i];
        const function: u16 = @truncate(entry.address);
        const device: u16 = @intCast(entry.address >> 16);
        if (function != 0xFFFF and function != info.addr.function) continue;
        if (device != 0xFFFF and device != info.addr.device) continue;
        if (info.interrupt_pin != entry.pin) continue;

        // If there is no link device, then entry.index can be treated as GSI
        data.@"1" = entry.index;
        if (entry.source) |source| {
            if (uacpi.uacpi_for_each_device_resource(source, "_CRS", get_handler_prt_resource_callback, &data.@"1") != uacpi.UACPI_STATUS_OK) {
                log.warn("Failed to iterate over resources of link device", .{});
                return uacpi.UACPI_ITERATION_DECISION_BREAK;
            }
        }
    }
    return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;
}

fn get_handler_prt_resource_callback(_gsi: ?*anyopaque, _resource: [*c]uacpi.uacpi_resource) callconv(.c) uacpi.uacpi_iteration_decision {
    const gsi: *u32 = @ptrCast(@alignCast(_gsi));
    const resource: *uacpi.uacpi_resource = @ptrCast(_resource);
    switch (resource.type) {
        uacpi.UACPI_RESOURCE_TYPE_IRQ => {
            const irq = resource.unnamed_0.irq;
            if (irq.num_irqs > 1) {
                gsi.* = irq.irqs()[0];
                return uacpi.UACPI_ITERATION_DECISION_BREAK;
            }
        },
        uacpi.UACPI_RESOURCE_TYPE_EXTENDED_IRQ => {
            const irq = resource.unnamed_0.extended_irq;
            if (irq.num_irqs > 1) {
                gsi.* = irq.irqs()[0];
                return uacpi.UACPI_ITERATION_DECISION_BREAK;
            }
        },
        else => {},
    }
    return uacpi.UACPI_ITERATION_DECISION_CONTINUE;
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
