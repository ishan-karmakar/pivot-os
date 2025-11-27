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
    inline for (kernel.drivers.PCI_DRIVER_LIST) |driver| {
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

fn resource_callback(_: ?*anyopaque, _resource: [*c]uacpi.uacpi_resource) callconv(.c) uacpi.uacpi_iteration_decision {
    const resource: *uacpi.uacpi_resource = @ptrCast(_resource);
    switch (resource.type) {
        uacpi.UACPI_RESOURCE_TYPE_IRQ => {
            const irq = resource.unnamed_0.irq;
            for (0..irq.num_irqs) |i| {
                log.info("Possible GSI: {}", .{irq.irqs()[i]});
            }
        },
        uacpi.UACPI_RESOURCE_TYPE_EXTENDED_IRQ => {
            const irq = resource.unnamed_0.extended_irq;
            for (0..irq.num_irqs) |i| {
                log.info("Possible GSI: {}", .{irq.irqs()[i]});
            }
        },
        else => {},
    }
    return uacpi.UACPI_ITERATION_DECISION_CONTINUE;
}

fn callback(data: ?*anyopaque, node: ?*uacpi.uacpi_namespace_node, _: uacpi.uacpi_u32) callconv(.c) uacpi.uacpi_iteration_decision {
    const addr: *uacpi.uacpi_pci_address = @ptrCast(@alignCast(data));
    if (!uacpi.uacpi_device_matches_pnp_id(node, @ptrCast(&PCI_DEVICE_PNP_IDS))) return uacpi.UACPI_ITERATION_DECISION_CONTINUE;
    var bbn: u64 = 0;
    _ = uacpi.uacpi_eval_simple_integer(node, "_BBN", &bbn);
    if (bbn != addr.bus) return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;
    var prt: *uacpi.uacpi_pci_routing_table = undefined;
    if (uacpi.uacpi_get_pci_routing_table(node, @ptrCast(&prt)) != uacpi.UACPI_STATUS_OK)
        @panic("uACPI failed to get PCI routing table from node");
    for (0..prt.num_entries) |i| {
        const entry: uacpi.uacpi_pci_routing_table_entry = prt.entries()[i];
        const device: u16 = @intCast(entry.address >> 16);
        const function: u16 = @truncate(entry.address);
        if (device != addr.device) continue;
        if (function != 0xFFFF and (function != addr.function)) continue;
        const pin = read_reg8(addr.*, 0x3C + 1);
        if (pin == 0) {
            log.warn("PCI device does not use an interrupt pin", .{});
            return uacpi.UACPI_ITERATION_DECISION_BREAK;
        } else if (pin - 1 != entry.pin) continue;
        // var gsi = entry.index;
        if (entry.source) |source| {
            if (uacpi.uacpi_for_each_device_resource(source, "_CRS", resource_callback, null) != uacpi.UACPI_STATUS_OK)
                @panic("Failed to iterate over resources of link device");
        }
    }
    uacpi.uacpi_free_pci_routing_table(prt);
    // PCI devices cannot be nested, so no point of doing it recursively
    return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;
}

pub const RoutingType = union(enum) {
    MSI: void,
    MSIX: void,
    PRT: u32,
};

pub fn setup_handlers(addr: uacpi.uacpi_pci_address, handlers: []*idt.HandlerData) RoutingType {
    // Try MSI-X first, then return handler if success
    // Try MSI second, then return handler if success
    if (handlers.len != 1) @panic("Exactly one handler needed for PRT based routing");
    return .{ .PRT = get_handler_prt(addr, handlers[0]) };
}

// Get handler through PCI routing table
fn get_handler_prt(addr: uacpi.uacpi_pci_address, handler: *idt.HandlerData) u32 {
    var data = @Tuple(&.{ uacpi.uacpi_pci_address, u32 }){ addr, 0 };
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
    const data: *@Tuple(&.{ uacpi.uacpi_pci_address, u32 }) = @ptrCast(@alignCast(_data));
    const addr = data.@"0";

    // Check if the device is actually a PCI bus
    if (!uacpi.uacpi_device_matches_pnp_id(node, @ptrCast(&PCI_DEVICE_PNP_IDS)))
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Check if the bus number of PCI device is same as actual device (i.e. is device on this PCI bus?)
    var bbn: u64 = 0;
    _ = uacpi.uacpi_eval_simple_integer(node, "_BBN", &bbn);
    if (bbn != addr.bus)
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Get the PCI routing table
    var prt: *uacpi.uacpi_pci_routing_table = undefined;
    if (uacpi.uacpi_get_pci_routing_table(node, @ptrCast(&prt)) != uacpi.UACPI_STATUS_OK) {
        log.warn("Failed to get the PCI routing table from PCI device", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Get interrupt pin from the device's config space
    const pin = read_reg8(addr, 0x3C + 1);
    if (pin == 0) {
        log.warn("PCI device does not use an interrupt pin", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Iterate over the routing table entries in search for one that matches our device
    for (0..prt.num_entries) |i| {
        const entry: uacpi.uacpi_pci_routing_table_entry = prt.entries()[i];
        const function: u16 = @truncate(entry.address);
        const device: u16 = @intCast(entry.address >> 16);
        if (function != 0xFFFF and function != addr.function) continue;
        if (device != 0xFFFF and device != addr.device) continue;
        if (pin - 1 != entry.pin) continue;

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

pub fn find_cap(addr: uacpi.uacpi_pci_address, cap_id: u8) ?u8 {
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
