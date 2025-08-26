const kernel = @import("root");
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.rtl8139);
const uacpi = @import("uacpi");

pub var PCIVTable = pci.VTable{
    .target_codes = &.{.{ .vendor_id = 0x10EC, .device_id = 0x8139 }},
};

pub var PCITask = kernel.Task{
    .name = "RTL8139 Ethernet Driver",
    .init = init,
    .dependencies = &.{.{ .task = &kernel.drivers.acpi.NamespaceLoadTask }},
};

fn init() kernel.Task.Ret {
    pci.write_reg(PCIVTable.addr, 0x4, pci.read_reg(PCIVTable.addr, 0x4) | (1 << 2));
    const ioaddr: u16 = @intCast(pci.read_reg(PCIVTable.addr, 0x10) & 0xFFFFFFFC);
    serial.out(ioaddr + 0x52, @as(u8, 0));
    serial.out(ioaddr + 0x37, @as(u8, 0x10));
    while (serial.in(ioaddr + 0x37, u8) & 0x10 != 0) {}
    // TODO: Make buffer 1500 bytes instead of 0x3000 bytes
    serial.out(ioaddr + 0x30, @as(u32, @intCast(kernel.lib.mem.kmapper.translate(@intFromPtr((kernel.lib.mem.kvmm.allocator().alloc(u8, 1500) catch return .failed).ptr)).?)));

    var out_table: *uacpi.uacpi_pci_routing_table = undefined;
    log.info("{}", .{uacpi.uacpi_namespace_node_get_object(uacpi.uacpi_namespace_root()).?.*.type});
    log.info("{}", .{uacpi.uacpi_get_pci_routing_table(uacpi.uacpi_namespace_root(), @ptrCast(&out_table))});
    return .success;
}
