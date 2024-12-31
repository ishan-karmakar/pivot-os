const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);
const std = @import("std");

pub fn init_tables() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
    log.info("Initialized ACPI tables", .{});
}

pub fn get_table(T: type, sig: [*c]const u8) ?*const T {
    var tbl: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature(sig, &tbl) != uacpi.UACPI_STATUS_OK) {
        return null;
    }
    return @ptrCast(tbl.unnamed_0.hdr);
}

pub fn Iterator(T: type) type {
    return struct {
        id: u8,
        end: usize,
        cur: *const uacpi.acpi_entry_hdr,

        pub fn create(id: u8, table: *const uacpi.acpi_sdt_hdr, tbl_len: usize) @This() {
            return .{
                .id = id,
                .end = @intFromPtr(table) + table.length,
                .cur = @ptrFromInt(@intFromPtr(table) + tbl_len),
            };
        }

        pub fn next(self: *@This()) ?*const T {
            while (@intFromPtr(self.cur) < self.end) {
                const cur = self.cur;
                self.cur = @ptrFromInt(@intFromPtr(self.cur) + cur.length);
                if (cur.type == self.id) return @ptrCast(cur);
            }
            return null;
        }
    };
}
