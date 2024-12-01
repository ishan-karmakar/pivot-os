const MADT = @import("madt.zig");
const uacpi = @import("uacpi");
const log = @import("std").log.scoped(.acpi);
const std = @import("std");

pub var madt: MADT = undefined;
pub var hpet: ?*const uacpi.acpi_hpet = null;

pub fn init_tables() void {
    if (uacpi.uacpi_initialize(0) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_initialize failed");
    }
    madt = MADT.create(get_table(uacpi.acpi_madt, "APIC") orelse @panic("Could not find MADT"));
    hpet = get_table(uacpi.acpi_hpet, "HPET");
}

fn get_table(T: type, sig: [*c]const u8) ?*const T {
    var tbl: uacpi.uacpi_table = undefined;
    if (uacpi.uacpi_table_find_by_signature(sig, &tbl) != uacpi.UACPI_STATUS_OK) {
        return null;
    }
    return @ptrCast(tbl.unnamed_0.hdr);
}

pub fn init() void {
    if (uacpi.uacpi_namespace_initialize() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_namespace_initialize failed");
    }
    if (uacpi.uacpi_finalize_gpe_initialization() != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_finalize_gpe_initialization failed");
    }
    // TODO: Handle power button
    // TODO: Handle embedded controller
}

fn shutdown(_: uacpi.uacpi_handle) callconv(.C) noreturn {
    if (uacpi.uacpi_prepare_for_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_prepare_for_sleep_state failed");
    }

    asm volatile ("cli");
    if (uacpi.uacpi_enter_sleep_state(uacpi.UACPI_SLEEP_STATE_S5) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_enter_sleep_state failed");
    }
    unreachable;
}

fn handle_power_button(_: uacpi.uacpi_handle) callconv(.C) uacpi.uacpi_interrupt_ret {
    log.info("Power button pressed, shutting down...", .{});
    if (uacpi.uacpi_kernel_schedule_work(uacpi.UACPI_WORK_GPE_EXECUTION, shutdown, null) != uacpi.UACPI_STATUS_OK) {
        @panic("uacpi_kernel_schedule_work failed");
    }
    return uacpi.UACPI_INTERRUPT_HANDLED;
}
