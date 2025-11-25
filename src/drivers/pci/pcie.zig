const uacpi = @import("uacpi");
const kernel = @import("root");
const pci = kernel.drivers.pci;

pub var Task = kernel.Task{
    .init = init,
    .name = "PCIe",
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KMapperTask },
        .{ .task = &kernel.drivers.acpi.TablesTask },
    },
};

const CONFIG_SPACE_PAGES: comptime_int = ((255 << 20) | (31 << 15) | (7 << 12) + 0x1000) / 0x1000;
var segment_groups: []const uacpi.acpi_mcfg_allocation = undefined;

fn init() kernel.Task.Ret {
    const tbl = kernel.drivers.acpi.get_table(uacpi.acpi_mcfg, uacpi.ACPI_MCFG_SIGNATURE) orelse return .failed;
    const num_entries = (tbl.hdr.length - @sizeOf(uacpi.acpi_mcfg)) / @sizeOf(uacpi.acpi_mcfg_allocation);
    const groups: [*]const uacpi.acpi_mcfg_allocation = tbl.entries();
    segment_groups = groups[0..num_entries];
    // pci.read_reg = read_reg;
    // pci.write_reg = write_reg;

    for (segment_groups) |seg| {
        for (0..CONFIG_SPACE_PAGES) |i| {
            kernel.lib.mem.kmapper.map(seg.address + i * 0x1000, seg.address + i * 0x1000, (1 << 63) | 0b11);
        }
        pci.scan_devices(seg.segment);
    }
    return .success;
}

fn get_addr(base: usize, addr: uacpi.uacpi_pci_address, off: u13) usize {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);
    return base + ((bus << 20) | (device << 15) | (func << 12)) + off;
}

pub fn read_reg(addr: uacpi.uacpi_pci_address, off: u13) u32 {
    for (segment_groups) |seg| {
        if (seg.segment == addr.segment) return get_addr(seg.address, addr, off).*;
    }
    @panic("PCIe segment not found");
}

pub fn write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: anytype) void {
    for (segment_groups) |seg| {
        if (seg.segment == addr.segment)
            @as(*@TypeOf(val), @ptrFromInt(get_addr(seg.address, addr, off))).* = val;
    }
    @panic("PCIe segment not found");
}
