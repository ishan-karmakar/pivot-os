const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);

pub fn init() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
    if (uacpi.uacpi_namespace_load() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_load failed");
    }
    if (uacpi.uacpi_namespace_initialize() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_initialize failed");
    }
    if (uacpi.uacpi_finalize_gpe_initialization() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_finalize_gpe_initialization failed");
    }
}

pub fn shutdown() void {
    if (uacpi.uacpi_prepare_for_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_prepare_for_sleep_state failed");
    }

    asm volatile ("cli");
    if (uacpi.uacpi_enter_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_enter_sleep_state failed");
    }
    unreachable;
}
