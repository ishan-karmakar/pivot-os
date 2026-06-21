const kernel = @import("root");
const timers = kernel.timers;
const serial = kernel.drivers.serial;
const std = @import("std");
const log = std.log.scoped(.acpi_timer);
const uacpi = @import("uacpi");

pub var Task = kernel.Task{
    .name = "ACPI Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.acpi.TablesTask },
    },
};

const CLOCKSOURCE = timers.ClockSource{
    .rating = 200,
    .read = time,
};

const HZ = 3579545;

var address: usize = undefined;
var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "ACPI Timer");
    kernel.drivers.acpi.init_tables() catch |err|
        return kernel.lib.logger.failed_initialization(log, "ACPI Timer", err);

    const fadt = kernel.drivers.acpi.get_table(uacpi.acpi_fadt, "FACP") catch |err|
        return kernel.lib.logger.failed_initialization(log, "ACPI Timer", err);
    if (fadt.pm_tmr_len != 4) {
        log.err("fadt.pm_tmr_len ({}) != 4", .{fadt.pm_tmr_len});
        return kernel.lib.logger.failed_initialization(log, "ACPI Timer", error.PMTmrLenIncorrectLength);
    }
    var rsdp_addr: u64 = undefined;
    if (uacpi.uacpi_kernel_get_rsdp(@ptrCast(&rsdp_addr)) != uacpi.UACPI_STATUS_OK)
        return kernel.lib.logger.failed_initialization(log, "ACPI Timer", error.RSDPNotFound);
    const rsdp: *uacpi.acpi_rsdp = @ptrFromInt(rsdp_addr);
    address = fadt.x_pm_tmr_blk.address;
    if (!(rsdp.revision == 2 and address != 0)) {
        if (fadt.pm_tmr_blk != 0) {
            address = @intCast(fadt.pm_tmr_blk);
        } else return kernel.lib.logger.failed_initialization(log, "ACPI Timer", error.FADTPMTmrBlkAddrNotFound);
    }

    timers.register_clocksource(&CLOCKSOURCE);
    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "ACPI Timer");
}

fn read_ticks() u32 {
    if (address <= std.math.maxInt(u32)) {
        return @intCast(serial.in(@intCast(address), u32));
    } else {
        return @intCast(@as(*const u32, @ptrFromInt(address)).*);
    }
}

pub fn time() usize {
    return @as(usize, @intCast(read_ticks())) * 1_000_000_000 / HZ;
}
