const limine = @import("limine");
const std = @import("std");
const mem = @import("kernel").lib.mem;
const cpu = @import("kernel").drivers.cpu;

pub export var SMP_REQUEST: limine.SmpRequest = .{ .flags = 1 };

pub const CPU = struct {
    id: usize,
    ready: bool,
};

pub var cpus: []CPU = undefined;

pub fn init() void {
    cpus = mem.kheap.allocator().alloc(CPU, SMP_REQUEST.response.?.cpu_count) catch @panic("OOM");
    for (0..SMP_REQUEST.response.?.cpu_count) |i| {
        const info = SMP_REQUEST.response.?.cpus()[i];
        const cpu_info = cpus + i;
        info.extra_argument = @intFromPtr(cpu_info);
        cpu_info.id = info.lapic_id;
        if (info.lapic_id == SMP_REQUEST.response.?.bsp_lapic_id) cpu.set_kgs(info.extra_argument);
    }
}

pub fn cur_cpu() *CPU {
    return @ptrFromInt(cpu.get_kgs());
}
