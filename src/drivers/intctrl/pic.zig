const kernel = @import("kernel");
const log = @import("std").log.scoped(.pic);
const serial = kernel.drivers.serial;
const VTable = kernel.drivers.intctrl.VTable;
const acpi = kernel.drivers.acpi;
const idt = kernel.drivers.idt;

const PIC1 = 0x20;
const PIC2 = 0xA0;
const PIC1_DATA = PIC1 + 1;
const PIC2_DATA = PIC2 + 1;
const PIC_EOI = 0x20;

pub const vtable: VTable = .{
    .init = init,
    .set = set,
    .mask = mask,
    .eoi = eoi,
};

fn init() bool {
    if (acpi.madt.table.flags & 1 == 0) {
        log.debug("Dual 8259 Legacy PICs not installed", .{});
        return false;
    }
    for (0x20..0x30) |v| idt.vec2handler(@intCast(v)).reserved = true;
    disable();
    serial.out(PIC1, @as(u8, 0x10 | 0x1));
    serial.out(PIC2, @as(u8, 0x10 | 0x1));
    serial.out(PIC1_DATA, @as(u8, 0x20));
    serial.out(PIC2_DATA, @as(u8, 0x20 + 8));
    serial.out(PIC1_DATA, @as(u8, 4));
    serial.out(PIC2_DATA, @as(u8, 2));
    serial.out(PIC1_DATA, @as(u8, 1));
    serial.out(PIC2_DATA, @as(u8, 1));

    log.info("Initialized 8259 PIC", .{});
    return true;
}

fn set(vec: u8, irq: u5, _: u64) void {
    if (vec != @as(u32, @intCast(irq)) + 0x20) @panic("vector != (irq + 0x20)");
}

fn eoi(irq: u5) void {
    serial.out(if (irq < 8) PIC1 else PIC2, @as(u8, PIC_EOI));
}

fn mask(_irq: u5, m: bool) void {
    var irq = _irq;
    const port = get_port(&irq);
    if (m) {
        serial.out(port, serial.in(port, u8) | (@as(u8, 1) << @intCast(irq)));
    } else {
        serial.out(port, serial.in(port, u8) & ~(@as(u8, 1) << @intCast(irq)));
    }
}

fn disable() void {
    serial.out(PIC1_DATA, @as(u8, 0xFF));
    serial.out(PIC2_DATA, @as(u8, 0xFF));
}

fn get_port(irq: *u5) u16 {
    if (irq.* < 8) return PIC1_DATA;
    irq.* -= 8;
    return PIC2_DATA;
}
