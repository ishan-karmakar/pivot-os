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

pub fn eoi(_irq: u8) void {
    var irq = _irq;
    var port = block: {
        if (irq < 8) break :block PIC1_DATA;
        irq -= 8;
        break :block PIC2_DATA;
    };
}

fn disable() void {
    serial.out(PIC1_DATA, @as(u8, 0xFF));
    serial.out(PIC2_DATA, @as(u8, 0xFF));
}
