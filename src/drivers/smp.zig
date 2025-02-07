const kernel = @import("kernel");
const limine = @import("limine");
const std = @import("std");
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const log = std.log.scoped(.smp);

pub export var SMP_REQUEST: limine.SmpRequest = .{ .flags = 1, .revision = 3 };

pub const CPU = struct {
    id: u32,
    ready: std.atomic.Value(bool),
    timeslice: usize = 0,
};

pub var Task = kernel.Task{
    .name = "SMP",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KHeapTask },
        .{ .task = &kernel.drivers.gdt.DynamicTask },
    },
};

var TaskAP = kernel.Task{
    .name = "SMP (AP)",
    .init = null,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.TaskAP },
        .{ .task = &kernel.drivers.gdt.DynamicTaskAP },
    },
};

pub fn init() kernel.Task.Ret {
    const response = SMP_REQUEST.response orelse return .failed;
    for (0..response.cpu_count) |i| {
        const info = response.cpus()[i];
        const _info = mem.kheap.allocator().create(CPU) catch return .failed;
        _info.* = CPU{
            .id = @truncate(i),
            .ready = std.atomic.Value(bool).init(false),
        };
        info.extra_argument = @intFromPtr(_info);
        if (info.lapic_id == SMP_REQUEST.response.?.bsp_lapic_id) {
            cpu.set_kgs(info.extra_argument);
        } else {
            info.goto_address = ap_init;
            while (!_info.ready.load(.acquire)) {}
            // This is hack, because the APs are reusing the same task
            // This means that we need to be EXTREMELY careful about using TaskAP outside of this file
            // Because it only reflects the status of the last CPU that ran the task
            TaskAP.ret = null;
        }
    }
    return .success;
}

fn ap_init(info: *limine.SmpInfo) callconv(.C) noreturn {
    kernel.drivers.cpu.set_kgs(info.extra_argument);
    TaskAP.run();
    if (TaskAP.ret != .success) @panic("SMP AP Task failed");
    asm volatile ("sti");
    cpu_info(null).ready.store(true, .monotonic);
    while (true) asm volatile ("hlt");
}

/// Gets CPU info of {idx} cpu
/// If idx is null, gets current cpu info
pub fn cpu_info(idx: ?usize) *CPU {
    if (idx) |i| return @ptrFromInt(SMP_REQUEST.response.?.cpus_ptr[i].extra_argument);
    return @ptrFromInt(cpu.get_kgs());
}

pub inline fn cpu_count() usize {
    return SMP_REQUEST.response.?.cpu_count;
}
