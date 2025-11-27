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
    pci.read_reg8 = GenerateReadFunc(u8);
    pci.read_reg16 = GenerateReadFunc(u16);
    pci.read_reg32 = GenerateReadFunc(u32);
    pci.write_reg8 = GenerateWriteFunc(u8);
    pci.write_reg16 = GenerateWriteFunc(u16);
    pci.write_reg32 = GenerateWriteFunc(u32);

    for (segment_groups) |seg| {
        for (0..CONFIG_SPACE_PAGES) |i| {
            kernel.lib.mem.kmapper.map(seg.address + i * 0x1000, seg.address + i * 0x1000, (1 << 63) | 0b11);
        }
        pci.scan_devices(seg.segment) catch return .failed;
    }
    return .success;
}

fn get_addr(base: usize, addr: uacpi.uacpi_pci_address, off: u13) usize {
    const bus: u32 = @intCast(addr.bus);
    const device: u32 = @intCast(addr.device);
    const func: u32 = @intCast(addr.function);
    return base + ((bus << 20) | (device << 15) | (func << 12)) + off;
}

fn GenerateReadFunc(T: type) pci.ReadFunc(T) {
    return struct {
        fn read_reg(addr: uacpi.uacpi_pci_address, off: u13) T {
            for (segment_groups) |seg| {
                if (seg.segment == addr.segment) return @as(*T, @ptrFromInt(get_addr(seg.address, addr, off))).*;
            }
            @panic("PCIe segment not found");
        }
    }.read_reg;
}

fn GenerateWriteFunc(T: type) pci.WriteFunc(T) {
    return struct {
        fn write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: T) void {
            for (segment_groups) |seg| {
                if (seg.segment == addr.segment) {
                    @as(*T, @ptrFromInt(get_addr(seg.address, addr, off))).* = val;
                    return;
                }
            }
            @panic("PCIe segment not found");
        }
    }.write_reg;
}
pub fn write_reg(addr: uacpi.uacpi_pci_address, off: u13, val: anytype) void {
    for (segment_groups) |seg| {
        if (seg.segment == addr.segment)
            @as(*@TypeOf(val), @ptrFromInt(get_addr(seg.address, addr, off))).* = val;
    }
    @panic("PCIe segment not found");
}
