const kernel = @import("root");
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
    vendor_id: ?u16 = null,
    device_id: ?u16 = null,
};

pub const VTable = struct {
    target_codes: []const Codes,
    addr: uacpi.uacpi_pci_address = undefined,
};

const AVAILABLE_DRIVERS = [_]type{
    // kernel.drivers.ide,
};

pub var Task = kernel.Task{
    .name = "PCI(e)",
    .init = init,
    .dependencies = &.{.{ .task = &kernel.drivers.acpi.NamespaceTask }},
};

pub var read_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13) u32 = pci_read_reg;
pub var write_reg: *const fn (addr: uacpi.uacpi_pci_address, off: u13, val: u32) void = pci_write_reg;
var segment_groups: []const uacpi.acpi_mcfg_allocation = undefined;
var routing_table = std.ArrayList(*const uacpi.uacpi_pci_routing_table_entry).empty;

fn init() kernel.Task.Ret {
    const hids: [*]const [*c]const u8 = &.{
        "PNP0A03",
        "PNP0A08",
        0,
    };
    _ = uacpi.uacpi_find_devices_at(
        uacpi.uacpi_namespace_root(),
        hids,
        iterate_host_bridges,
        null,
    );

    const pcie_table = acpi.get_table(uacpi.acpi_mcfg, uacpi.ACPI_MCFG_SIGNATURE);
    if (pcie_table) |tbl| init_pcie(tbl);

    if (read_reg == pci_read_reg) {
        scan_devices(0);
    } else for (segment_groups) |seg| {
        scan_devices(seg.segment);
    }

    _ = allocate_irq(.{
        .segment = 0,
        .bus = 0,
        .device = 3,
        .function = 0,
    });
    return .success;
}

fn iterate_host_bridges(_: ?*anyopaque, node: [*c]uacpi.uacpi_namespace_node, _: u32) callconv(.c) uacpi.uacpi_iteration_decision {
    const path = uacpi.uacpi_namespace_node_generate_absolute_path(node);
    uacpi.uacpi_free_absolute_path(path);
    var rt: *uacpi.uacpi_pci_routing_table = undefined;
    if (uacpi.uacpi_get_pci_routing_table(node, @ptrCast(&rt)) != uacpi.UACPI_STATUS_OK) @panic("Failed to get PCI routing table of host bridge");
    for (0..rt.num_entries) |i| routing_table.append(kernel.lib.mem.kheap.allocator(), @ptrCast(&rt.entries()[i])) catch @panic("OOM");
    // We can't free them because the routing table entries are still being used
    return uacpi.UACPI_ITERATION_DECISION_CONTINUE;
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
    const vendor_id: u16 = @truncate(vendor_dev);
    const device_id: u16 = @truncate(vendor_dev >> 16);
    log.debug(
        "Found function at address {} (VID: 0x{x}, DID: 0x{x}, CC: 0x{x}, SC: 0x{x}, PIF: 0x{x})",
        .{
            addr,
            vendor_id,
            device_id,
            class_code,
            subclass_code,
            prog_if,
        },
    );
    inline for (AVAILABLE_DRIVERS) |driver| {
        for (driver.PCIVTable.target_codes) |code| {
            if (code.class_code) |cc| if (cc != class_code) continue;
            if (code.subclass_code) |scc| if (scc != subclass_code) continue;
            if (code.prog_if) |pif| if (pif != prog_if) continue;
            if (code.vendor_id) |vid| if (vid != vendor_id) continue;
            if (code.device_id) |did| if (did != device_id) continue;
            driver.PCIVTable.addr = addr;
            driver.PCITask.run();
        }
    }
    return true;
}

fn init_pcie(tbl: *const uacpi.acpi_mcfg) void {
    log.info("Using PCIe", .{});
    const num_entries = (tbl.hdr.length - @sizeOf(uacpi.acpi_mcfg)) / @sizeOf(uacpi.acpi_mcfg_allocation);
    const groups: [*]const uacpi.acpi_mcfg_allocation = tbl.entries();
    segment_groups = groups[0..num_entries];

    read_reg = pcie_read_reg;
    write_reg = pcie_write_reg;

    for (segment_groups) |seg| {
        for (0..PCIE_CONFIG_SPACE_PAGES) |i| {
            kernel.lib.mem.kmapper.map(seg.address + i * 0x1000, seg.address + i * 0x1000, (1 << 63) | 0b11);
        }
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

pub fn pcie_get_addr(base: usize, addr: uacpi.uacpi_pci_address, off: u13) *u32 {
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
        if (seg.segment == addr.segment) {
            pcie_get_addr(seg.address, addr, off).* = val;
            return;
        }
    }
    @panic("PCIe segment not found");
}

// This allocates a legacy IRQ for a device using the PCI routing table and ACPI resources
// If there are multiple buses, this WILL break
// TODO: Fix when multiple buses, _BBN?
// Returns the GSI
pub fn allocate_irq(addr: uacpi.uacpi_pci_address) u32 {
    const pin: u8 = @truncate(read_reg(addr, 0x3C) >> 8);
    log.debug("Device is using interrupt pin {}", .{pin});
    for (routing_table.items) |entry| if (entry.pin == pin) {
        const entry_device: u16 = @truncate(entry.address >> 16);
        const entry_function: u16 = @truncate(entry.address);
        if (addr.function != entry_function and entry_function != 0xFFFF) continue;
        if (addr.device != entry_device and entry_device != 0xFFFF) continue;
        if (entry.source == null) return entry.index;

        // First check the device's current resources. If any non-zero IRQs are listed, we can use them
        var irq: ?u32 = null;
        if (uacpi.uacpi_for_each_device_resource(entry.source, "_CRS", iterate_resources, &irq) != uacpi.UACPI_STATUS_OK) @panic("Could not iterate over current device resources");
        var resources: *uacpi.uacpi_resources = undefined;
        if (uacpi.uacpi_get_possible_resources(entry.source, @ptrCast(&resources)) != uacpi.UACPI_STATUS_OK) @panic("Could not get possible resources of device");
        var resource: *uacpi.uacpi_resource = resources.entries;
        const old_size = resources.length;
        while (true) {
            switch (resource.type) {
                uacpi.UACPI_RESOURCE_TYPE_EXTENDED_IRQ => {
                    for (0..resource.unnamed_0.extended_irq.num_irqs) |i| {
                        const extended_irq = resource.unnamed_0.extended_irq;
                        if (extended_irq.irqs()[i] == 0) continue;
                        resources.length = 48 + 8;
                        resources.entries = resource;
                        const end_tag: *uacpi.uacpi_resource = @ptrFromInt(@intFromPtr(resource) + 48);
                        end_tag.type = uacpi.UACPI_RESOURCE_TYPE_END_TAG;
                        end_tag.length = 8;
                        break;
                    }
                },
                uacpi.UACPI_RESOURCE_TYPE_END_TAG => break,
                else => {},
            }
            resource = @ptrFromInt(@intFromPtr(resource) + resource.length);
        }
        if (uacpi.uacpi_set_resources(entry.source, resources) != uacpi.UACPI_STATUS_OK) @panic("Failed to set resources");
        resources.length = old_size;
        uacpi.uacpi_free_resources(resources);
        log.info("", .{});
        if (uacpi.uacpi_for_each_device_resource(entry.source, "_CRS", iterate_resources, &irq) != uacpi.UACPI_STATUS_OK) @panic("Could not iterate over current device resources");
        // if (uacpi.uacpi_set_resources(entry.source, resources) != uacpi.UACPI_STATUS_OK) @panic("Failed to set resources");
        // uacpi.uacpi_free_resources(resources);
    };
    @panic("No routing table entry found for device");
}

fn iterate_resources(_user: ?*anyopaque, resource: [*c]uacpi.uacpi_resource) callconv(.c) uacpi.uacpi_iteration_decision {
    const user: *?u32 = @ptrCast(@alignCast(_user));
    switch (resource.*.type) {
        uacpi.UACPI_RESOURCE_TYPE_IRQ => {
            const irq = resource.*.unnamed_0.irq;
            log.info("Current resources: {}", .{irq});
            for (0..irq.num_irqs) |i| {
                log.info("Found IRQ {}", .{i});
                if (irq.irqs()[i] != 0) {
                    user.* = irq.irqs()[i];
                    return uacpi.UACPI_ITERATION_DECISION_BREAK;
                }
            }
        },
        uacpi.UACPI_RESOURCE_TYPE_EXTENDED_IRQ => {
            const irq = resource.*.unnamed_0.extended_irq;
            log.info("Current resources: {}", .{irq});
            for (0..irq.num_irqs) |i| {
                log.info("Found IRQ {}", .{i});
                if (irq.irqs()[i] != 0) {
                    user.* = irq.irqs()[i];
                    return uacpi.UACPI_ITERATION_DECISION_BREAK;
                }
            }
        },
        else => {},
    }
    return uacpi.UACPI_ITERATION_DECISION_CONTINUE;
}
