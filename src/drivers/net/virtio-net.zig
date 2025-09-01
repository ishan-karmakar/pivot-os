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

const VirtIOCap = packed struct {
    cap_vndr: u8,
    cap_next: u8,
    cap_len: u8,
    cfg_type: u8,
    bar: u8,
    id: u8,
    padding: u16,
    offset: u32,
    length: u32,
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
    log.debug("Found capabilities list at offset {}", .{cap_off});

    while (true) {
        const tmp = pci.read_reg(PCIVTable.addr, cap_off);
        const id: u8 = @truncate(tmp);
        const next_ptr: u8 = @truncate(tmp >> 8);
        if (id == 9) { // Vendor specific ID
            const cfg_type: u8 = @truncate(tmp >> 24);
            const generic_len: u8 = @truncate(tmp >> 16);
            const length = pci.read_reg(PCIVTable.addr, cap_off + 12);
            log.info("Found VirtIO capability structure, type: {}", .{cfg_type});
            log.info("Generic length: {}, length: {}", .{ generic_len, length });
        }

        if (next_ptr == 0) break;
        cap_off = next_ptr;
    }
}
