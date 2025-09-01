const kernel = @import("root");
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.virtio_net);
const uacpi = @import("uacpi");

pub var PCITask = kernel.Task{
    .name = "VirtIO Network Driver",
    .init = init,
    .dependencies = &.{},
};

pub var PCIVTable = pci.VTable{ .target_codes = &.{
    .{ .vendor_id = 0x1af4 },
} };

const VirtIOCommonCFG = packed struct {
    device_feature_select: u32,
    device_feature: u32,
    driver_feature_select: u32,
    driver_feature: u32,
    config_msix_vector: u16,
    num_queues: u16,
    device_status: u8,
    config_generation: u8,

    queue_select: u16,
    queue_size: u16,
    queue_msix_vector: u16,
    queue_enable: u16,
    queue_notify_off: u16,
    queue_desc: u64,
    queue_driver: u64,
    queue_device: u64,
    queue_notif_config_data: u16,
    queue_reset: u16,

    admin_queue_index: u16,
    admin_queue_num: u16,
};

fn init() kernel.Task.Ret {
    if (!is_virtio_net()) {
        log.debug("After further probing, PCI device is not network card, skipping", .{});
        return .skipped;
    }
    traverse_capability_list() catch return .failed;

    return .success;
}

// Checks whether the device is actually a VirtIO Network device
// VirtIO drivers are distinguished via their Subsystem ID in the PCI Config space, not by their device ID
fn is_virtio_net() bool {
    const subsystem_id: u16 = @truncate(pci.read_reg(PCIVTable.addr, 0x2C) >> 16);
    return subsystem_id == 1;
}

fn traverse_capability_list() !void {
    const capability_list_enabled = (pci.read_reg(PCIVTable.addr, 0x4) >> 16) & (1 << 4) > 0;
    if (!capability_list_enabled) return error.CapabilityListNotEnabled;

    var cap_off: u8 = @truncate(pci.read_reg(PCIVTable.addr, 0x34));

    while (true) {
        const tmp = pci.read_reg(PCIVTable.addr, cap_off);
        const id: u8 = @truncate(tmp);
        const next_ptr: u8 = @truncate(tmp >> 8);
        if (id == 9) { // Vendor specific ID
            switch (@as(u8, @truncate(tmp >> 24))) {
                1 => handle_common_cfg(cap_off),
                else => {},
            }
        }

        if (next_ptr == 0) break;
        cap_off = next_ptr;
    }
}

fn handle_common_cfg(cap_off: u8) void {
    const bar = switch (pci.read_bar(PCIVTable.addr, @truncate(pci.read_reg(PCIVTable.addr, cap_off + 4)))) {
        .IO => @panic("Received IO BAR when expecting a MEM BAR"),
        .MEM => |v| v,
    };
    kernel.lib.mem.kmapper.map(bar, bar, 0b11 | (1 << 63));
    const common_cfg: *volatile VirtIOCommonCFG = @ptrFromInt(bar);
    common_cfg.device_feature_select = 0;
    log.info("Device feature: {b}", .{common_cfg.device_feature});
}
