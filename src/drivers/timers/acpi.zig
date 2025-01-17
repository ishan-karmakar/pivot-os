const kernel = @import("kernel");

pub var Task = kernel.Task{
    .name = "ACPI Timer",
    .dependencies = &.{
        .{ .task = &kernel.drivers.acpi.TablesTask, .accept_failure = true },
    },
};
