const kernel = @import("root");
const pci = kernel.drivers.pci;
const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.msi);

pub var Task = kernel.Task{
    .name = "MSI",
    .init = null,
    .dependencies = &.{
        .{ .task = &kernel.lib.smp.Task },
    },
};

const CAP_ID = 5;

pub fn enable_msi_single(addr: uacpi.uacpi_pci_address) bool {
    const cap = pci.find_cap(addr, CAP_ID) orelse return false;
    const msg_ctrl = pci.read_reg16(addr, cap + 2);
    const dest_apic_id = kernel.lib.smp.cpu_info(null).id;
    const msg_addr_low = 0xFEE00000 | (dest_apic_id << 12);
    pci.write_reg32(addr, cap + 4, msg_addr_low);
    var next_off: u13 = cap + 8;
    if (msg_ctrl & (1 << 7) != 0) {
        pci.write_reg32(addr, cap + 8, 0);
        next_off += 4;
    }
    const delivery_mode = 0;
    const trigger_level = 0;
    const handler = kernel.drivers.idt.allocate_handler(null);
    const vector = kernel.drivers.idt.handler2vec(handler);
    const msg_data = @as(u16, vector) | (delivery_mode << 8) | (trigger_level << 15);
    pci.write_reg16(addr, next_off, msg_data);

    pci.write_reg16(addr, cap + 2, msg_ctrl | 1);
    log.info("Enabled MSI for {}", .{addr});
    return true;
}
