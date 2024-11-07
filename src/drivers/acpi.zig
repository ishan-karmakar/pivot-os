const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);

pub fn init() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
}

pub fn init2() void {
    if (uacpi.uacpi_namespace_load() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_load failed");
    }
    if (uacpi.uacpi_namespace_initialize() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_initialize failed");
    }
}
