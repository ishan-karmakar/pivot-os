const log = @import("std").log.scoped(.pic);
const serial = @import("kernel").drivers.serial;

const PIC1 = 0x20;
const PIC2 = 0xA0;
const PIC1_DATA = PIC1 + 1;
const PIC2_DATA = PIC2 + 1;
const PIC_EOI = 0x20;

pub fn init() void {
    asm volatile ("cli");
    serial.out(PIC1, @as(u8, 0x10 | 0x1));
    serial.out(PIC2, @as(u8, 0x10 | 0x1));
    serial.out(PIC1_DATA, @as(u8, 0x20));
    serial.out(PIC2_DATA, @as(u8, 0x20 + 8));
    serial.out(PIC1_DATA, @as(u8, 4));
    serial.out(PIC2_DATA, @as(u8, 2));
    serial.out(PIC1_DATA, @as(u8, 1));
    serial.out(PIC2_DATA, @as(u8, 1));
    disable();
    log.info("Initialized 8259 PIC", .{});
}

pub inline fn eoi(irq: u8) void {
    serial.out(if (irq < 8) PIC1 else PIC2, @as(u8, PIC_EOI));
}

pub fn mask(_irq: u8) void {
    var irq = _irq;
    const port = get_port(&irq);
    serial.out(port, serial.in(port, u8) | (@as(u8, 1) << @intCast(irq)));
}

pub fn unmask(_irq: u8) void {
    var irq = _irq;
    const port = get_port(&irq);
    serial.out(port, serial.in(port, u8) & ~(@as(u8, 1) << @intCast(irq)));
}

fn get_port(irq: *u8) u16 {
    if (irq.* < 8) return PIC1_DATA;
    irq.* -= 8;
    return PIC2_DATA;
}

pub fn disable() void {
    serial.out(PIC1_DATA, @as(u8, 0xFF));
    serial.out(PIC2_DATA, @as(u8, 0xFF));
}
