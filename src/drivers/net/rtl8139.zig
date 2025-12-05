const kernel = @import("root");
const log = @import("std").log.scoped(.rtl8139);
const pci = kernel.drivers.pci;
const serial = kernel.drivers.serial;
const cpu = kernel.drivers.cpu;
const lwip = @import("lwip");
const std = @import("std");

const IORegisters = struct {
    pub const CONFIG_1 = 0x52;
    pub const CMD = 0x37;
    pub const RBSTART = 0x30;
    pub const IMR = 0x3C;
    pub const RCR = 0x44;
    pub const MAC_LOW = 0;
    pub const MAC_HIGH = 4;
    pub const ISR = 0x3E;
    pub const CBR = 0x3A;
    pub const CAPR = 0x38;
};

const InterruptStatusRegister = packed struct {
    ROK: bool,
    RER: bool,
    TOK: bool,
    TER: bool,
    RXOVW: bool,
    PUN: bool,
    FOVW: bool,
    reserved: u6,
    LenChg: bool,
    TimeOut: bool,
    SERR: bool,
};

const Header = packed struct {
    status: packed struct {
        ROK: bool,
        FAE: bool,
        CRC: bool,
        LONG: bool,
        RUNT: bool,
        ISE: bool,
        reserved: u7,
        BAR: bool,
        PAM: bool,
        MAR: bool,
    },
    len: u16,
};

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
    .dependencies = &.{.{ .task = &kernel.drivers.lwip.Task }},
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
    serial.out(io_base + IORegisters.CONFIG_1, @as(u8, 0));

    serial.out(io_base + IORegisters.CMD, @as(u8, 0x10));
    while (serial.in(io_base + IORegisters.CMD, u8) & 0x10 != 0) {}

    const num_pages = std.math.divCeil(usize, BUFFER_SIZE, 0x1000) catch return .failed;
    const pages = kernel.lib.mem.pmm.frames(num_pages);
    for (0..num_pages) |i|
        kernel.lib.mem.kmapper.map(pages + 0x1000 * i, kernel.lib.mem.virt(pages + 0x1000 + i), 0b11 | (1 << 63));
    buffer = @as([*]u8, @ptrFromInt(kernel.lib.mem.virt(pages)))[0..BUFFER_SIZE];

    serial.out(io_base + IORegisters.RBSTART, @as(u32, @intCast(pages)));
    serial.out(io_base + IORegisters.IMR, @as(u16, 0x5));
    serial.out(io_base + IORegisters.RCR, @as(u32, 0b1110 | (1 << 7)));
    serial.out(io_base + IORegisters.CMD, @as(u8, 0x0C));

    var netif = lwip.netif{};
    var ipaddr = lwip.ip4_addr_t{ .addr = 0xC0A85C64 }; // 192.168.86.100
    var netmask = lwip.ip4_addr_t{ .addr = 0xFFFFFF00 };
    var gw = lwip.ip4_addr_t{};
    _ = lwip.netif_add(&netif, &ipaddr, &netmask, &gw, null, if_init, lwip.ethernet_input);

    return .success;
}

fn if_init(_netif: [*c]lwip.netif) callconv(.c) lwip.err_t {
    const netif: *lwip.netif = @ptrCast(_netif);
    netif.hwaddr = get_mac();
    netif.hwaddr_len = 6;
    return lwip.ERR_OK;
}

fn get_mac() [6]u8 {
    const mac_low = serial.in(io_base + IORegisters.MAC_LOW, u32);
    const mac_high = serial.in(io_base + IORegisters.MAC_HIGH, u16);
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
    var status: InterruptStatusRegister = @bitCast(serial.in(io_base + IORegisters.ISR, u16));

    // Check for receive OK
    if (status.ROK) {
        while (serial.in(io_base + IORegisters.CBR, u16) != rx_offset) {
            const header = @as(*const Header, @ptrFromInt(@intFromPtr(buffer.ptr) + rx_offset)).*;
            if (header.status.ROK) {
                log.info("Received packet with length: {} and status {}", .{ header.len, header.status });
                if (header.status.BAR) {
                    log.info("Received broadcast packet", .{});
                } else if (header.status.PAM) {
                    log.info("Received unicast packet", .{});
                } else if (header.status.MAR) {
                    log.info("Received multicast packet", .{});
                }
            } else {
                log.debug("Frame is corrupt", .{});
            }

            rx_offset = ((rx_offset + header.len + @sizeOf(Header) + 3) & ~@as(u16, 3)) % BUFFER_SIZE;
            serial.out(io_base + IORegisters.CAPR, rx_offset);
        }
        log.info("Finished receiving packet", .{});

        // clear ROK bit in ISR
        status = @bitCast(@as(u16, 0));
        status.ROK = true;
        serial.out(io_base + IORegisters.ISR, @as(u16, @bitCast(status)));
    }

    return cpu_status;
}
