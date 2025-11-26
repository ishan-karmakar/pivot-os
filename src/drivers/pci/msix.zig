const kernel = @import("root");
const uacpi = @import("uacpi");
const pci = kernel.drivers.pci;
const std = @import("std");
const log = std.log.scoped(.msix);
const idt = kernel.drivers.idt;

pub var Task = kernel.Task{
    .name = "MSI",
    .init = null,
    .dependencies = &.{},
};

const CAP_ID = 0x11;

const TableEntry = extern struct {
    msg_addr_low: u32,
    msg_addr_high: u32,
    msg_data: u32,
    vec_ctrl: u32,
};

pub fn enable(addr: uacpi.uacpi_pci_address) ![]*idt.HandlerData {
    const cap = pci.find_cap(addr, CAP_ID) orelse return error.Unsupported;
    const msg_ctrl = pci.read_reg16(addr, cap + 2);
    const table_info = pci.read_reg32(addr, cap + 4);
    const table_bir: u3 = @truncate(table_info);
    const table_offset = table_info >> 3;

    const table_size = (msg_ctrl & 0x7FF) + 1;

    const table = switch (pci.find_bar(addr, table_bir)) {
        .MEM => |bar| blk: {
            const table_addr = bar.addr + table_offset;
            const num_pages = std.math.divCeil(usize, table_addr % 0x1000 + table_size * @sizeOf(TableEntry), 0x1000) catch unreachable;
            for (0..num_pages) |i|
                kernel.lib.mem.kmapper.map(table_addr + i * 0x1000, table_addr + i * 0x1000, (1 << 63) | 0b11);
            break :blk @as([*]volatile TableEntry, @ptrFromInt(table_addr))[0..table_size];
        },
        .IO => @panic("IO BARs are not allowed with MSI-X"),
    };
    const id = kernel.lib.smp.cpu_info(null).id;
    const handlers = try std.ArrayList(*idt.HandlerData).initCapacity(kernel.lib.mem.kheap.allocator(), table_size);
    for (table) |*ent| {
        ent.msg_addr_low = 0xFEE00000 | (id << 12);
        ent.msg_addr_high = 0;
        const handler = kernel.drivers.idt.allocate_handler(null);
        try handlers.append(kernel.lib.mem.kheap.allocator(), handler);
        ent.msg_data = kernel.drivers.idt.handler2vec(handler);
        ent.vec_ctrl = 0;
    }

    pci.write_reg16(addr, cap + 2, (msg_ctrl | (1 << 15)) & ~(@as(u16, 1) << 14));
    log.info("Enabled MSI-X on {}", .{addr});
    return try handlers.toOwnedSlice();
}
