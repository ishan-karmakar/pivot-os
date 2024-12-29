const kernel = @import("kernel");
const acpi = kernel.drivers.acpi;
const timers = kernel.drivers.timers;
const uacpi = @import("uacpi");

pub const vtable = timers.VTable{
    .init = init,
    .sleep = sleep,
    .time = time,
};

var initialized: ?bool = null;

fn init() bool {
    defer initialized = initialized orelse false;
    if (initialized) |i| return i;

    if (acpi.fadt == null) return false;
    const f = acpi.fadt.?;

    if (f.pm_tmr_len != 4) return false;
    // check ACPI revision >= 2
    //

    return true;
}

fn time() usize {}

fn sleep(ns: usize) void {
    _ = ns;
}
