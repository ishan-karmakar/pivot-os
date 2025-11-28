const kernel = @import("root");
const pci = kernel.drivers.pci;
const idt = kernel.drivers.idt;
const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.prt);

const PCI_DEVICE_PNP_IDS = [_][*c]const u8{
    "PNP0A03",
    "PNP0A08",
    @ptrCast(uacpi.UACPI_NULL),
};

const Data = struct {
    info: pci.DeviceInfo,
    gsi: u32 = 0,
};

pub fn setup_handler(info: pci.DeviceInfo, handler: *idt.HandlerData) u32 {
    var data = Data{ .info = info };
    if (uacpi.uacpi_namespace_for_each_child(
        uacpi.uacpi_namespace_get_predefined(uacpi.UACPI_PREDEFINED_NAMESPACE_SB),
        device_callback,
        null,
        uacpi.UACPI_OBJECT_DEVICE_BIT,
        // TODO: Could restrict to depth 1?
        uacpi.UACPI_MAX_DEPTH_ANY,
        &data,
        // &addr,
    ) != uacpi.UACPI_STATUS_OK) @panic("Failed to iterate over PCI devices");
    _ = kernel.drivers.intctrl.map(idt.handler2vec(handler), data.gsi) catch @panic("Failed to map PCI device's GSI");
    return data.gsi;
}

fn device_callback(_data: ?*anyopaque, node: ?*uacpi.uacpi_namespace_node, _: uacpi.uacpi_u32) callconv(.c) uacpi.uacpi_iteration_decision {
    const data: *Data = @ptrCast(@alignCast(_data));

    // Check if the device is actually a PCI bus
    if (!uacpi.uacpi_device_matches_pnp_id(node, @ptrCast(&PCI_DEVICE_PNP_IDS)))
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Check if the bus number of PCI device is same as actual device (i.e. is device on this PCI bus?)
    var bbn: u64 = 0;
    _ = uacpi.uacpi_eval_simple_integer(node, "_BBN", &bbn);
    if (bbn != data.info.addr.bus)
        return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;

    // Get the PCI routing table
    var prt: *uacpi.uacpi_pci_routing_table = undefined;
    if (uacpi.uacpi_get_pci_routing_table(node, @ptrCast(&prt)) != uacpi.UACPI_STATUS_OK) {
        log.warn("Failed to get the PCI routing table from PCI device", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Get interrupt pin from the device's config space
    if (data.info.interrupt_pin == null) {
        log.warn("PCI device does not use an interrupt pin", .{});
        return uacpi.UACPI_ITERATION_DECISION_BREAK;
    }

    // Iterate over the routing table entries in search for one that matches our device
    for (0..prt.num_entries) |i| {
        const entry: uacpi.uacpi_pci_routing_table_entry = prt.entries()[i];
        const function: u16 = @truncate(entry.address);
        const device: u16 = @intCast(entry.address >> 16);
        if (function != 0xFFFF and function != data.info.addr.function) continue;
        if (device != 0xFFFF and device != data.info.addr.device) continue;
        if (data.info.interrupt_pin != entry.pin) continue;

        // If there is no link device, then entry.index can be treated as GSI
        data.gsi = entry.index;
        if (entry.source) |source| {
            if (uacpi.uacpi_for_each_device_resource(source, "_CRS", resource_callback, &data.gsi) != uacpi.UACPI_STATUS_OK) {
                log.warn("Failed to iterate over resources of link device", .{});
                return uacpi.UACPI_ITERATION_DECISION_BREAK;
            }
        }
    }
    return uacpi.UACPI_ITERATION_DECISION_NEXT_PEER;
}

fn resource_callback(_gsi: ?*anyopaque, _resource: [*c]uacpi.uacpi_resource) callconv(.c) uacpi.uacpi_iteration_decision {
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
