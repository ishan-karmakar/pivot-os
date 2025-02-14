const kernel = @import("kernel");
const uacpi = @import("uacpi");
const pci = kernel.drivers.pci;
const acpi = kernel.drivers.acpi;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.ide);

pub var PCIVTable = pci.VTable{
    .target_codes = &.{
        .{ .class_code = 0x1, .subclass_code = 0x1 },
    },
    .target_ret = undefined,
};

pub var PCITask = kernel.Task{
    .name = "IDE PCI Driver",
    .init = init,
    .dependencies = &.{},
};

const ATA_PRIMARY = 0;
const ATA_SECONDARY = 1;

const ChannelRegisters = struct {
    base: u32,
    ctrl: u32,
    bmide: u32,
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

fn init() kernel.Task.Ret {
    const bmide = get_bmide(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr);
    const channels = [2]ChannelRegisters{
        .{
            .base = get_primary_channel(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
            .ctrl = get_primary_channel_control_port(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
            .bmide = bmide,
        },
        .{
            .base = get_secondary_channel(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
            .ctrl = get_secondary_channel_control_port(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
            .bmide = bmide + 8,
        },
    };
    _ = channels;

    return .success;
}

inline fn get_primary_channel(prog_if: u8, addr: uacpi.uacpi_pci_address) u32 {
    return if (prog_if & 1 == 0) 0x1F0 else get_bar(addr, 0x10);
    // TODO: Use bit 1 to modify pci native/compatibility
}

inline fn get_primary_channel_control_port(prog_if: u8, addr: uacpi.uacpi_pci_address) u32 {
    return if (prog_if & 1 == 0) 0x3F6 else get_bar(addr, 0x14);
    // TODO: Use bit 1 to modify pci native/compatibility
}

inline fn get_secondary_channel(prog_if: u8, addr: uacpi.uacpi_pci_address) u32 {
    return if (prog_if & (1 << 2) == 0) 0x170 else get_bar(addr, 0x18);
}

inline fn get_secondary_channel_control_port(prog_if: u8, addr: uacpi.uacpi_pci_address) u32 {
    return if (prog_if & (1 << 2) == 0) 0x376 else get_bar(addr, 0x1C);
}

inline fn get_bmide(prog_if: u8, addr: uacpi.uacpi_pci_address) u32 {
    return if (prog_if & (1 << 7) == 0) 0 else get_bar(addr, 0x20);
}

fn get_bar(addr: uacpi.uacpi_pci_address, off: u8) u32 {
    const bar = pci.read_reg(addr, off);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return bar & 0xFFFFFFFC;
}
