const kernel = @import("kernel");
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.ide);

pub const vtable = pci.VTable{
    .target_codes = &.{
        .{ .class_code = 0x1, .subclass_code = 0x1 },
    },
    .init = init,
};

const ChannelRegisters = struct {
    base: u16,
    cntrl: u16,
    bmide: u16,
    nIEN: u8,
};

const Device = packed struct {
    exists: u8,
    channel: u8,
    drive: u8,
    dtype: u16,
    sig: u16,
    cap: u16,
    cmdsets: u32,
    size: u32,
    model: [41]u8,
};

fn init(segment: u16, bus: u8, device: u5, func: u3) void {
    const prog_if: u8 = @truncate(pci.read_reg(segment, bus, device, func, 0x8) >> 8);
    const primary_channel = get_primary_channel(prog_if, segment, bus, device, func);
    const primary_channel_cntrl_port = get_primary_channel_control_port(prog_if, segment, bus, device, func);
    const secondary_channel = get_secondary_channel(prog_if, segment, bus, device, func);
    const secondary_channel_cntrl_port = get_secondary_channel_control_port(prog_if, segment, bus, device, func);
}

fn get_primary_channel(prog_if: u8, segment: u16, bus: u8, device: u5, func: u3) u32 {
    if (prog_if & 1 == 0) return 0x1F0;
    // TODO: Use bit 1 to modify pci native/compatibility

    const bar = pci.read_reg(segment, bus, device, func, 0x10);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return bar & 0xFFFFFFFC;
}

fn get_primary_channel_control_port(prog_if: u8, segment: u16, bus: u8, device: u5, func: u3) u32 {
    if (prog_if & (1 << 1) == 0) return 0x3F6;
    // TODO: Use bit 1 to modify pci native/compatibility

    const bar = pci.read_reg(segment, bus, device, func, 0x14);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return bar & 0xFFFFFFFC;
}

fn get_secondary_channel(prog_if: u8, segment: u16, bus: u8, device: u5, func: u3) u32 {
    if (prog_if & (1 << 3) == 0) return 0x170;
    // TODO: Use bit 3 to modify pci native/compatibility

    const bar = pci.read_reg(segment, bus, device, func, 0x18);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return bar & 0xFFFFFFFC;
}

fn get_secondary_channel_control_port(prog_if: u8, segment: u16, bus: u8, device: u5, func: u3) u32 {
    if (prog_if & (1 << 3) == 0) return 0x376;
    // TODO: Use bit 3 to modify pci native/compatibility

    const bar = pci.read_reg(segment, bus, device, func, 0x1C);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return bar & 0xFFFFFFFC;
}
