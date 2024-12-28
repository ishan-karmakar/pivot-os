const mem = @import("kernel").lib.mem;
const uacpi = @import("uacpi");
const ArrayList = @import("std").ArrayList;

const IOAPICArrayList = ArrayList(*const uacpi.acpi_madt_ioapic);
const ISOArrayList = ArrayList(*const uacpi.acpi_madt_interrupt_source_override);

table: *const uacpi.acpi_madt,
ioapics: IOAPICArrayList,
isos: ISOArrayList,

pub fn create(tbl: *const uacpi.acpi_madt) @This() {
    var self: @This() = .{
        .table = tbl,
        .ioapics = IOAPICArrayList.init(mem.kheap.allocator()),
        .isos = ISOArrayList.init(mem.kheap.allocator()),
    };
    var ent: *const uacpi.acpi_entry_hdr = @ptrCast(tbl.entries());
    while (true) {
        if (@intFromPtr(ent) >= (@intFromPtr(tbl) + tbl.hdr.length)) break;
        self.handle_entry(ent);
        ent = @ptrFromInt(@intFromPtr(ent) + ent.length);
    }
    return self;
}

fn handle_entry(self: *@This(), ent: *const uacpi.acpi_entry_hdr) void {
    switch (ent.type) {
        uacpi.ACPI_MADT_ENTRY_TYPE_IOAPIC => self.ioapics.append(@ptrCast(ent)) catch @panic("OOM"),
        uacpi.ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE => self.isos.append(@ptrCast(ent)) catch @panic("OOM"),
        else => {},
    }
}
