const pic = @import("kernel").drivers.pic;
const uacpi = @import("uacpi");
const mem = @import("kernel").lib.mem;
const log = @import("std").log.scoped(.ioapic);

const RedirectionEntry = extern struct {
    vec: u8,
    delivery_mode: u3,
    dest_mode: u1 = 0,
    delivery_status: bool = false,
    pin_polarity: u1,
    remote_irr: u1 = 0, // IDK what this is
    trigger_mode: u1,
    mask: bool = true,
    rsv: u39 = 0,
    dest: u8,
};

var addr: ?usize = null;
var madt: *const uacpi.acpi_madt = undefined; // Just caching MADT here

pub fn init() void {
    pic.disable();
    var out_table: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature("APIC", @ptrCast(&out_table)) != uacpi.UACPI_STATUS_OK) {
        @panic("uACPI error finding MADT");
    }
    madt = @ptrCast(out_table.unnamed_0.hdr);
    var hdr: *const uacpi.acpi_entry_hdr = @ptrFromInt(@intFromPtr(madt) + @sizeOf(uacpi.acpi_madt));
    while (true) {
        if (hdr.type == uacpi.ACPI_MADT_ENTRY_TYPE_IOAPIC) break;
        hdr = @ptrFromInt(@intFromPtr(hdr) + hdr.length);
        if (@intFromPtr(hdr) >= @intFromPtr(madt) + madt.hdr.length) @panic("Could not find IOAPIC entry in MADT");
    }
    const ioapic: *const uacpi.acpi_madt_ioapic = @ptrCast(hdr);
    log.debug("Found I/O APIC entry in MADT: {}", .{ioapic});
    // TODO: Do we alert PMM to reserve address? Does the limine memory map overlap with LAPIC / IOAPIC?
    addr = @intCast(ioapic.address);
    mem.kmapper.map(addr.?, addr.?, (@as(u64, 1) << 63) | 0b10);
    // We don't need to transfer and interrupts over
    // The only thing used was PIT, and we are able to use LAPIC timer at this stage
    log.info("Initialized I/O APIC", .{});
}

pub fn set(vec: u8, _irq: u8, dest: u8, flags: u64) void {
    var irq = _irq;
    var ent = @as(RedirectionEntry, flags);
    ent.vec = vec;
    ent.dest = dest;
    if (find_so(irq)) |so| {
        log.debug("Found interrupt source override for IRQ {}: {}", .{ irq, so });
        irq = so.gsi;
        ent.pin_polarity = @intFromBool(so.flags & 2 > 0);
        ent.trigger_mode = @intFromBool(so.flags & 8 > 0);
    }

    write_red(irq, ent);
    log.debug("Setting IRQ {} to IDT entry {}", .{ irq, vec });
}

pub fn mask(_irq: u8, m: bool) void {
    const irq = if (find_so(_irq)) |i| @as(u8, i.gsi) else _irq;
    var ent = read_red(irq);
    ent.mask = m;
    write_red(irq, ent);
}

fn write_red(_irq: u8, ent: RedirectionEntry) void {
    const irq = _irq * 2 + 0x10;
    const lower: u32 = @truncate(@as(u64, ent));
    const upper: u32 = @truncate(@as(u64, ent) >> 32);

    write_reg(irq, lower);
    write_reg(irq, upper);
}

fn read_red(_irq: u8) RedirectionEntry {
    const irq = _irq * 2 + 0x10;
    return @as(RedirectionEntry, @as(u64, @intCast(read_reg(irq))) | read_reg(irq + 1));
}

fn write_reg(off: u32, val: u32) void {
    const a = addr orelse @panic("I/O APIC addr has not been determined yet");
    @as(*u32, @ptrFromInt(a)).* = off;
    @as(*u32, @ptrFromInt(a + 0x10)).* = val;
}

fn read_reg(off: u32) u32 {
    const a = addr orelse @panic("I/O APIC addr has not been determined yet");
    @as(*u32, @ptrFromInt(a)).* = off;
    return @as(*u32, @ptrFromInt(a + 0x10)).*;
}

fn find_so(irq: u8) ?*const uacpi.acpi_madt_interrupt_source_override {
    var hdr: *const uacpi.acpi_entry_hdr = @ptrFromInt(@intFromPtr(madt) + @sizeOf(uacpi.acpi_madt));
    while (true) {
        if (hdr.type == uacpi.ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
            const so: *const uacpi.acpi_madt_interrupt_source_override = @ptrCast(hdr);
            if (so.source == irq) return so;
        }
        hdr = @ptrFromInt(@intFromPtr(hdr) + hdr.length);
        if (@intFromPtr(hdr) >= @intFromPtr(madt) + madt.hdr.length) return null;
    }
}
