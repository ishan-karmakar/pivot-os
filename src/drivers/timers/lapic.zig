const kernel = @import("kernel");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;

pub const vtable = timers.TimerVTable{
    .deinit = deinit,
};

pub var Task = kernel.Task{
    .name = "Local APIC Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &lapic.Task, .accept_failure = true },
    },
};

fn init() bool {
    if (!lapic.Task.ret.?) return false;

    return true;
}

fn deinit() void {}
