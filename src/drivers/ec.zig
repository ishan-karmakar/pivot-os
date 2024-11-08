const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.ec);
const std = @import("std");

pub fn init() void {
    var out_table: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature("ECDT", &out_table) != uacpi.UACPI_STATUS_OK) {
        log.info("No ECDT detected", .{});
        return;
    }
    // TODO: Figure this out, not in QEMU
    const ecdt: *uacpi.acpi_ecdt = @ptrCast(out_table.unnamed_0.hdr);
    log.info("Found ECDT, {s}", .{std.mem.span(ecdt.ec_id())});
}
