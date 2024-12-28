const kernel = @import("kernel");
const intctrl = kernel.drivers.intctrl;
const acpi = kernel.drivers.acpi;
const lapic = kernel.drivers.lapic;
const log = @import("std").log.scoped(.ioapic);
const uacpi = @import("uacpi");
const std = @import("std");

pub const vtable = intctrl.VTable{
    .init = init,
    .map = map,
    .unmap = unmap,
    .mask = mask,
    .eoi = eoi,
};

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

const IOAPIC = struct {
    max_red_ent: usize,
    data: *const uacpi.acpi_madt_ioapic,

    pub fn create(data: *const uacpi.acpi_madt_ioapic) @This() {
        kernel.lib.mem.kmapper.map(data.address, data.address, (@as(u64, 1) << 63) | 0b10);
        var self = @This(){ .data = data, .max_red_ent = 0 };
        self.max_red_ent = ((self.read_reg(1) >> 16) & 0xFF) + 1;
        for (0..self.max_red_ent) |i| {
            self.write_red(i, @bitCast(@as(u64, 0)));
        }
        log.info("Initialized I/O APIC (GSI Base: {}, Max Redirection Entries: {})", .{ data.gsi_base, self.max_red_ent });
        return self;
    }

    pub fn supports_irq(self: @This(), irq: usize) bool {
        return self.data.gsi_base <= irq and irq < (self.data.gsi_base + self.max_red_ent);
    }

    pub fn write_red(self: @This(), _irq: usize, ent: RedirectionEntry) void {
        const irq = _irq * 2 + 0x10;
        const lower: u32 = @truncate(@as(u64, @bitCast(ent)));
        const upper: u32 = @truncate(@as(u64, @bitCast(ent)) >> 32);
        self.write_reg(@intCast(irq), lower);
        self.write_reg(@intCast(irq + 1), upper);
    }

    pub fn read_red(self: @This(), _irq: usize) RedirectionEntry {
        const irq = _irq * 2 + 0x10;
        const lower = self.read_reg(@intCast(irq));
        const upper = self.read_reg(@intCast(irq + 1));
        return @bitCast((@as(u64, upper) << 32) | lower);
    }

    fn write_reg(self: @This(), off: u32, val: u32) void {
        @as(*volatile u32, @ptrFromInt(self.data.address)).* = off;
        @as(*volatile u32, @ptrFromInt(self.data.address + 0x10)).* = val;
    }

    fn read_reg(self: @This(), off: u32) u32 {
        @as(*volatile u32, @ptrFromInt(self.data.address)).* = off;
        return @as(*const volatile u32, @ptrFromInt(self.data.address + 0x10)).*;
    }
};

var ioapics: std.ArrayList(IOAPIC) = undefined;

// TODO: Combine this ioapics and MADT ioapics together
fn init() bool {
    if (acpi.madt.ioapics.items.len == 0) {
        log.debug("No I/O APICs installed", .{});
        return false;
    }

    if (intctrl.pic.vtable.init()) {
        for (0x20..0x30) |v| kernel.drivers.idt.vec2handler(@intCast(v)).reserved = false;
    }

    ioapics = std.ArrayList(IOAPIC).initCapacity(kernel.lib.mem.kheap.allocator(), acpi.madt.ioapics.items.len) catch @panic("OOM");
    for (acpi.madt.ioapics.items) |ioapic| ioapics.appendAssumeCapacity(IOAPIC.create(ioapic));
    return true;
}

fn map(vec: u8, _irq: usize) !usize {
    var irq = _irq;
    var ent = RedirectionEntry{
        .vec = vec,
        .delivery_mode = 0,
        .dest_mode = 0,
        .delivery_status = false,
        .pin_polarity = 0,
        .remote_irr = 0,
        .trigger_mode = 0,
        .mask = false,
        .rsv = 0,
        .dest = 0, // TODO: map should take in destination
    };
    if (find_so(irq)) |so| {
        log.debug("Found interrupt source override for IRQ {} -> {}", .{ irq, so.gsi });
        irq = @intCast(so.gsi);
        ent.pin_polarity = @intFromBool(so.flags & 2 > 0);
        ent.trigger_mode = @intFromBool(so.flags & 8 > 0);
    }
    log.debug("Redirection entry {} -> {}", .{ irq, vec });
    for (ioapics.items) |ioapic| {
        if (ioapic.supports_irq(irq)) {
            if (@as(u64, @bitCast(ioapic.read_red(irq))) != 0) return error.IRQUsed;
            ioapic.write_red(irq, ent);
            return irq;
        }
    }
    return error.OutOfIRQs;
}

fn unmap(irq: usize) void {
    for (ioapics.items) |ioapic| {
        if (ioapic.supports_irq(irq)) {
            ioapic.write_red(irq, @bitCast(@as(u64, 0)));
        }
    }
}

fn find_so(irq: usize) ?*const uacpi.acpi_madt_interrupt_source_override {
    for (acpi.madt.isos.items) |so| if (so.source == irq) return so;
    return null;
}

fn mask(_irq: usize, m: bool) void {
    const irq: usize = if (find_so(_irq)) |i| @intCast(i.gsi) else _irq;
    for (ioapics.items) |ioapic| {
        if (ioapic.supports_irq(irq)) {
            var ent = ioapic.read_red(irq);
            ent.mask = m;
            ioapic.write_red(irq, ent);
        }
    }
}

fn eoi(_: usize) void {
    lapic.eoi();
}
