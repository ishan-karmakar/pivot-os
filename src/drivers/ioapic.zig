const kernel = @import("kernel");
const pic = kernel.drivers.pic;
const uacpi = @import("uacpi");
const lapic = kernel.drivers.lapic;
const acpi = kernel.drivers.acpi;
const mem = kernel.lib.mem;
const idt = kernel.drivers.idt;
const log = @import("std").log.scoped(.ioapic);

const RedirectionEntry = packed struct {
    vec: u8,
    delivery_mode: u3,
    dest_mode: u1 = 0,
    delivery_status: bool,
    pin_polarity: u1,
    remote_irr: u1 = 0, // IDK what this is
    trigger_mode: u1,
    mask: bool,
    rsv: u39,
    dest: u8,
};

var addr: usize = undefined;

pub fn init() bool {
    if (acpi.madt.ioapics.items.len == 0) {
        log.debug("No I/O APICs installed", .{});
        return false;
    } else if (acpi.madt.ioapics.items.len > 1) {
        log.debug("Number of I/O APICs > 1, unimplemented", .{});
        return false;
    }
    if (pic.init()) {
        for (0x20..0x30) |i| idt.get_handler(@intCast(i)).reserved = false;
    }
    const ioapic = acpi.madt.ioapics.items[0];
    // TODO: Do we alert PMM to reserve address? Does the limine memory map overlap with LAPIC / IOAPIC?
    addr = @intCast(ioapic.address);
    mem.kmapper.map(addr, addr, (@as(u64, 1) << 63) | 0b10);
    log.info("Initialized I/O APIC", .{});
    return true;
}

pub fn set(vec: u8, _irq: u5, flags: u64) void {
    var irq = _irq;
    var ent: RedirectionEntry = @bitCast(flags);
    ent.vec = vec;
    ent.mask = true;
    if (find_so(irq)) |so| {
        log.debug("Found interrupt source override for IRQ {} -> {}", .{ irq, so.gsi });
        irq = @intCast(so.gsi);
        ent.pin_polarity = @intFromBool(so.flags & 2 > 0);
        ent.trigger_mode = @intFromBool(so.flags & 8 > 0);
    }
    log.debug("Redirection entry {} -> {}", .{ irq, vec });
    write_red(irq, ent);
}

pub fn mask(_irq: u5, m: bool) void {
    const irq = if (find_so(_irq)) |i| @as(u8, @intCast(i.gsi)) else _irq;
    var ent = read_red(irq);
    ent.mask = m;
    write_red(irq, ent);
}

pub fn eoi(_: u5) void {
    lapic.eoi();
}

fn write_red(_irq: u8, ent: RedirectionEntry) void {
    const irq = _irq * 2 + 0x10;
    const lower: u32 = @truncate(@as(u64, @bitCast(ent)));
    const upper: u32 = @truncate(@as(u64, @bitCast(ent)) >> 32);

    write_reg(irq, lower);
    write_reg(irq + 1, upper);
}

fn read_red(_irq: u8) RedirectionEntry {
    const irq = _irq * 2 + 0x10;
    const lower = read_reg(irq);
    const upper = read_reg(irq + 1);
    return @bitCast((@as(u64, upper) << 32) | lower);
}

fn write_reg(off: u32, val: u32) void {
    @as(*volatile u32, @ptrFromInt(addr)).* = off;
    @as(*volatile u32, @ptrFromInt(addr + 0x10)).* = val;
}

fn read_reg(off: u32) u32 {
    @as(*volatile u32, @ptrFromInt(addr)).* = off;
    return @as(*volatile u32, @ptrFromInt(addr + 0x10)).*;
}

fn find_so(irq: u8) ?*const uacpi.acpi_madt_interrupt_source_override {
    for (acpi.madt.isos.items) |so| if (so.source == irq) return so;
    return null;
}
