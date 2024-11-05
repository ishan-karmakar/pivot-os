const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);

pub fn init() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
}
