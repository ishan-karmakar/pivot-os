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

const DeviceStatus = enum(u8) {
    RESET = 0,
    ACKNOWLEDGE = 1,
    DRIVER = 2,
    FAILED = 128,
    FEATURES_OK = 8,
    DRIVER_OK = 4,
    DEVICE_NEEDS_RESET = 64,
};

const Features = struct {
    const CSUM = 1 << 0;
    const GUEST_CSUM = 1 << 1;
    const CTRL_GUEST_OFFLOADS = 1 << 2;
    const MTU = 1 << 3;
    const MAC = 1 << 5;
    const GUEST_TSO4 = 1 << 7;
    const GUEST_TSO6 = 1 << 8;
    const GUEST_ECN = 1 << 9;
    const GUEST_UFO = 1 << 10;
    const HOST_TSO4 = 1 << 11;
    const HOST_TSO6 = 1 << 12;
    const HOST_ECN = 1 << 13;
    const HOST_UFO = 1 << 14;
    const MRG_RXBUF = 1 << 15;
    const STATUS = 1 << 16;
    const CTRL_VQ = 1 << 17;
    const CTRL_RX = 1 << 18;
    const CTRL_VLAN = 1 << 19;
    const CTRL_RX_EXTRA = 1 << 20;
    const GUEST_ANNOUNCE = 1 << 21;
    const MQ = 1 << 22;
    const CTRL_MAC_ADDR = 1 << 23;
    const INDIRECT_DESC = 1 << 28;
    const EVENT_IDX = 1 << 29;
    const VERSION_1 = 1 << 32;
    const ACCESS_PLATFORM = 1 << 33;
    const RING_PACKED = 1 << 34;
    const IN_ORDER = 1 << 35;
    const ORDER_PLATFORM = 1 << 36;
    const SR_IOV = 1 << 37;
    const NOTIFICATION_DATA = 1 << 38;
    const NOTIF_CONFIG_DATA = 1 << 39;
    const RING_RESET = 1 << 40;
    const ADMIN_VQ = 1 << 41;
    const HASH_TUNNEL = 1 << 51;
    const VQ_NOTF_COAL = 1 << 52;
    const NOTF_COAL = 1 << 53;
    const GUEST_USO4 = 1 << 54;
    const GUEST_USO6 = 1 << 55;
    const HOST_USO = 1 << 56;
    const HASH_REPORT = 1 << 57;
    const GUEST_HDRLEN = 1 << 59;
    const RSS = 1 << 60;
    const RSC_EXT = 1 << 61;
    const STANDBY = 1 << 62;
    const SPEED_DUPLEX = 1 << 63;
};

const ConfigTypes = enum(u8) {
    COMMON = 1,
    NOTIFY = 2,
    ISR = 3,
    DEVICE = 4,
    PCI = 5,
    SHARED_MEMORY = 8,
    VENDOR = 9,
};

const CommonCFG = extern struct {
    device_feature_select: u32,
    device_feature: u32,
    driver_feature_select: u32,
    driver_feature: u32,
    config_msix_vector: u16,
    num_queues: u16,
    device_status: DeviceStatus,
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

const DeviceCFG = extern struct {
    mac: [6]u8,
    status: u16,
    max_virtqueue_pairs: u16,
    mtu: u16,
    speed: u32,
    duplex: u8,
    rss_max_key_size: u8,
    rss_max_mdirection_table_length: u16,
    supported_hash_types: u32,
    supported_tunnel_types: u32,
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
            switch (@as(ConfigTypes, @enumFromInt(@as(u8, @truncate(tmp >> 24))))) {
                ConfigTypes.COMMON => try handle_common_cfg(cap_off),
                ConfigTypes.DEVICE => try handle_device_cfg(cap_off),
                else => {},
            }
        }

        if (next_ptr == 0) break;
        cap_off = next_ptr;
    }
}

fn handle_common_cfg(cap_off: u8) !void {
    const bar = switch (pci.read_bar(PCIVTable.addr, @truncate(pci.read_reg(PCIVTable.addr, cap_off + 4)))) {
        .IO => return error.IOBarUnexpected,
        .MEM => |v| v + pci.read_reg(PCIVTable.addr, cap_off + 8),
    };
    kernel.lib.mem.kmapper.map(bar, bar, 0b11 | (1 << 63));
    const cfg: *volatile CommonCFG = @ptrFromInt(bar);
    reset_device(cfg);
    set_status(cfg, .ACKNOWLEDGE);
    set_status(cfg, .DRIVER);
    try negotiate_device_features(cfg);
    log.info("Device has {} virtqueues", .{cfg.num_queues});
    for (0..cfg.num_queues) |i| {
        cfg.queue_select = @intCast(i);
        setup_virtqueue(cfg);
    }
}

fn handle_device_cfg(cap_off: u8) !void {
    const bar = switch (pci.read_bar(PCIVTable.addr, @truncate(pci.read_reg(PCIVTable.addr, cap_off + 4)))) {
        .IO => return error.IOBarUnexpected,
        .MEM => |v| v + pci.read_reg(PCIVTable.addr, cap_off + 8),
    };
    kernel.lib.mem.kmapper.map(bar, bar, 0b11 | (1 << 63));
    const cfg: *volatile DeviceCFG = @ptrFromInt(bar);
    log.info("Device MAC address: {any}", .{cfg.mac});
}

fn negotiate_device_features(cfg: *volatile CommonCFG) !void {
    cfg.device_feature_select = 1;
    var device_feature: u64 = cfg.device_feature;
    device_feature <<= 32;
    cfg.device_feature_select = 0;
    device_feature |= cfg.device_feature;
    var driver_feature = device_feature;
    if (device_feature & Features.MAC == 0) return error.FeatureNotFound;
    driver_feature = Features.MAC;
    // driver_feature &= ~@as(u64, Features.CSUM | Features.MTU | Features.HASH_REPORT | Features.HASH_TUNNEL | Features.STATUS | Features.RING_RESET | Features.MQ | Features.RSS | Features.CTRL_VQ | Features.ADMIN_VQ);

    cfg.driver_feature_select = 0;
    cfg.driver_feature = @truncate(driver_feature);
    cfg.driver_feature_select = 1;
    cfg.driver_feature = @intCast(driver_feature >> 32);
    set_status(cfg, .FEATURES_OK);
    if (cfg.device_status != .FEATURES_OK) return error.DeviceFeaturesNotSupported;
}

// Assumes that the virtqueue is already selected before function is called
fn setup_virtqueue(cfg: *volatile CommonCFG) void {
    if (cfg.queue_size == 0) {
        log.debug("Virtqueue size is empty, queue does not exist...", .{});
        return;
    }

    log.info("Queue desc size: {}", .{16 * cfg.queue_size});
    log.info("Queue driver size: {}", .{6 + 2 * cfg.queue_size});
    log.info("Queue device size: {}", .{6 + 8 * cfg.queue_size});
}

fn reset_device(cfg: *volatile CommonCFG) void {
    set_status(cfg, .RESET);
    while (cfg.device_status != .RESET) asm volatile ("pause");
}

fn set_status(cfg: *volatile CommonCFG, status: DeviceStatus) void {
    log.debug("Setting device status to {}...", .{status});
    cfg.device_status = status;
}
