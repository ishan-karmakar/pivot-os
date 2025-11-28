const kernel = @import("root");
const log = @import("std").log.scoped(.rtl8139);
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;

const CONFIG_1 = 0x52;
const CMD = 0x37;
const RBSTART = 0x30;
const IMR = 0x3C;
const RCR = 0x44;

const BUFFER_SIZE = 8192 + 16;

pub var PCIVTable = pci.VTable{ .target_codes = &.{.{
    .class_code = 0x2,
    .subclass_code = 0,
    .prog_if = 0,
    .vendor_id = 0x10ec,
    .device_id = 0x8139,
}} };

pub var PCITask = kernel.Task{
    .name = "RTL8139 PCI Driver",
    .init = init,
    .dependencies = &.{},
};

var io_base: u16 = undefined;
var buffer: []u8 = undefined;

fn init() kernel.Task.Ret {
    switch (PCIVTable.info.bars[0]) {
        .IO => |io| io_base = @intCast(io),
        else => return .failed,
    }
    const handler = kernel.drivers.idt.allocate_handler(null);
    log.info("Interrupt pin, {?}", .{PCIVTable.info.interrupt_pin});
    var handlers = [_]*kernel.drivers.idt.HandlerData{handler};
    log.info("{}", .{pci.setup_handlers(PCIVTable.info, handlers[0..])});

    // PCIVTable.info.write_command(PCIVTable.info.command | (1 << 2));
    // serial.out(io_base + CONFIG_1, @as(u8, 0));

    // serial.out(io_base + CMD, @as(u8, 0x10));
    // while (serial.in(io_base + CMD, u8) & 0x10 != 0) {}

    // buffer = kernel.lib.mem.kvmm.allocator().alloc(u8, BUFFER_SIZE) catch return .failed;
    // serial.out(io_base + RBSTART, @as(u32, @intCast(kernel.lib.mem.kmapper.translate(@intFromPtr(buffer.ptr)) orelse return .failed)));

    // serial.out(io_base + IMR, @as(u16, 0x5));

    // serial.out(io_base + RCR, @as(u32, 0xF));

    // serial.out(io_base + CMD, 0x0C);

    return .success;
}
