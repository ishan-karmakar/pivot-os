const kernel = @import("kernel");
const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.pic);
const intctrl = kernel.drivers.intctrl;
const acpi = kernel.drivers.acpi;
const serial = kernel.drivers.serial;

const PIC1 = 0x20;
const PIC2 = 0xA0;
const PIC1_DATA = PIC1 + 1;
const PIC2_DATA = PIC2 + 1;
const PIC_EOI = 0x20;

pub const vtable = intctrl.VTable{
    .init = init,
    .map = map,
    .unmap = unmap,
    .mask = mask,
    .eoi = eoi,
};

var reserved: u16 = 0;

fn init() bool {
    const madt = acpi.get_table(uacpi.acpi_madt, uacpi.ACPI_MADT_SIGNATURE) orelse {
        log.debug("MADT not found", .{});
        return false;
    };
    if (madt.flags & 1 == 0) {
        log.debug("Dual 8259 Legacy PICs not installed", .{});
        return false;
    }

    for (0x20..0x30) |v| kernel.drivers.idt.vec2handler(@intCast(v)).reserved = true;

    disable();
    serial.out(PIC1, @as(u8, 0x10 | 1));
    serial.out(PIC2, @as(u8, 0x10 | 1));
    serial.out(PIC1_DATA, @as(u8, 0x20));
    serial.out(PIC2_DATA, @as(u8, 0x20 + 8));
    serial.out(PIC1_DATA, @as(u8, 4));
    serial.out(PIC2_DATA, @as(u8, 2));
    serial.out(PIC1_DATA, @as(u8, 1));
    serial.out(PIC2_DATA, @as(u8, 1));

    log.info("Initialized 8259 PIC", .{});
    return true;
}

fn map(vec: u8, irq: usize) !usize {
    if (vec != (irq + 0x20) or irq >= 16) return error.InvalidIRQ;

    if (reserved & (@as(u16, 1) << @intCast(irq)) != 0) return error.IRQUsed;
    reserved |= @as(u16, 1) << @intCast(irq);
    mask(irq, true);

    return irq;
}

fn unmap(irq: usize) void {
    reserved &= ~(@as(u16, 1) << @intCast(irq));
}

fn eoi(irq: usize) void {
    serial.out(if (irq < 8) PIC1 else PIC2, @as(u8, PIC_EOI));
}

fn mask(_irq: usize, m: bool) void {
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

fn get_port(irq: *usize) u16 {
    if (irq.* < 8) return PIC1_DATA;
    irq.* -= 8;
    return PIC2_DATA;
}
