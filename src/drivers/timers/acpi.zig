const kernel = @import("root");
const timers = kernel.drivers.timers;
const serial = kernel.drivers.serial;
const std = @import("std");
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

fn init() kernel.Task.Ret {
    const fadt = kernel.drivers.acpi.get_table(uacpi.acpi_fadt, "FACP") orelse return .failed;
    if (fadt.pm_tmr_len != 4) return .failed;
    var rsdp_addr: u64 = undefined;
    if (uacpi.uacpi_kernel_get_rsdp(@ptrCast(&rsdp_addr)) != uacpi.UACPI_STATUS_OK) return .failed;
    const rsdp: *uacpi.acpi_rsdp = @ptrFromInt(rsdp_addr);
    address = fadt.x_pm_tmr_blk.address;
    if (!(rsdp.revision == 2 and address != 0)) {
        if (fadt.pm_tmr_blk != 0) {
            address = @intCast(fadt.pm_tmr_blk);
        } else return .failed;
    }

    timers.register_clocksource(&CLOCKSOURCE);
    return .success;
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
