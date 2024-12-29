const MADT = @import("madt.zig");
const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);
const std = @import("std");

pub var madt: MADT = undefined;
pub var hpet: ?*const uacpi.acpi_hpet = null;
pub var fadt: ?*const uacpi.acpi_fadt = null;

pub fn init_tables() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
    madt = MADT.create(get_table(uacpi.acpi_madt, "APIC") orelse @panic("Could not find MADT"));
    hpet = get_table(uacpi.acpi_hpet, "HPET");
    fadt = get_table(uacpi.acpi_fadt, "FACP");
    log.info("Initialized ACPI tables", .{});
}

fn get_table(T: type, sig: [*c]const u8) ?*const T {
    var tbl: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature(sig, &tbl) != uacpi.UACPI_STATUS_OK) {
        return null;
    }
    return @ptrCast(tbl.unnamed_0.hdr);
}
