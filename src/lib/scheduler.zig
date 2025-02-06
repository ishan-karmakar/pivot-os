const kernel = @import("kernel");
const idt = kernel.drivers.idt;
const std = @import("std");
const log = std.log.scoped(.sched);

const QUANTUM = 50;

pub var Task = kernel.Task{
    .name = "Scheduler",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KHeapTask },
    },
};

var sched_vec: *idt.HandlerData = undefined;

fn init() kernel.Task.Ret {
    return .success;
}
