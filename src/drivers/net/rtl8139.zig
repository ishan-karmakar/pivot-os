const kernel = @import("root");
const log = @import("std").log.scoped(.rtl8139);
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;
const cpu = kernel.drivers.cpu;

const CONFIG_1 = 0x52;
const CMD = 0x37;
const RBSTART = 0x30;
const IMR = 0x3C;
const RCR = 0x44;

const BUFFER_SIZE = 8192 + 16 + 1500;

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
    handler.handler = irq_handler;
    var handlers = [_]*kernel.drivers.idt.HandlerData{handler};
    const routing = pci.setup_handlers(PCIVTable.info, handlers[0..]);
    switch (routing) {
        .PRT => |irq| {
            kernel.drivers.intctrl.mask(irq, false);
        },
        else => {},
    }

    PCIVTable.info.write_command(PCIVTable.info.command | (1 << 2));
    serial.out(io_base + CONFIG_1, @as(u8, 0));

    serial.out(io_base + CMD, @as(u8, 0x10));
    while (serial.in(io_base + CMD, u8) & 0x10 != 0) {}

    const page2 = kernel.lib.mem.pmm.frame();
    const page1 = kernel.lib.mem.pmm.frame();
    if (page1 + 0x1000 != page2) return .failed;
    kernel.lib.mem.kmapper.map(page1, kernel.lib.mem.virt(page1), 0b11 | (1 << 63));
    kernel.lib.mem.kmapper.map(page2, kernel.lib.mem.virt(page2), 0b11 | (1 << 63));
    buffer = @as([*]u8, @ptrFromInt(kernel.lib.mem.virt(page1)))[0..BUFFER_SIZE];

    serial.out(io_base + RBSTART, @as(u32, @intCast(page1)));
    serial.out(io_base + IMR, @as(u16, 0x5));
    serial.out(io_base + RCR, @as(u32, 0b1110 | (1 << 7)));
    serial.out(io_base + CMD, @as(u8, 0x0C));

    return .success;
}

fn get_mac() [6]u8 {
    const mac_low = serial.in(io_base, u32);
    const mac_high = serial.in(io_base + 4, u16);
    return .{
        @intCast(mac_high >> 8),
        @truncate(mac_high),
        @intCast(mac_low >> 24),
        @truncate(mac_low >> 16),
        @truncate(mac_low >> 8),
        @truncate(mac_low),
    };
}

var rx_offset: u16 = 0;

fn irq_handler(_: ?*anyopaque, cpu_status: *cpu.Status) *const cpu.Status {
    const status = serial.in(io_base + 0x3E, u16);

    // Check for receive OK
    if ((status & 1) != 0) {
        while (serial.in(io_base + 0x3A, u16) != rx_offset) {
            const packet_raw: [*]u16 = @ptrFromInt(@intFromPtr(buffer.ptr) + rx_offset);
            const packet_status = packet_raw[0];
            const packet_len = packet_raw[1];
            if (packet_status & 1 != 0) {
                log.info("Received packet with length: {} and status {}", .{ packet_len, packet_status });
                if (packet_status & (1 << 13) != 0) {
                    log.info("Received broadcast packet", .{});
                } else if (packet_status & (1 << 14) != 0) {
                    log.info("Received unicast packet", .{});
                } else if (packet_status & (1 << 15) != 0) {
                    log.info("Received multicast packet", .{});
                }
            } else {
                log.debug("Frame is corrupt", .{});
            }

            rx_offset = ((rx_offset + packet_len + 4 + 3) & ~@as(u16, 3)) % BUFFER_SIZE;
            serial.out(io_base + 0x38, rx_offset);
        }
        log.info("Finished receiving packet", .{});

        // clear ROK bit in ISR
        serial.out(io_base + 0x3E, @as(u16, 0x01));
    }

    return cpu_status;
}
