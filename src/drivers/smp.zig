const limine = @import("limine");
const std = @import("std");
const mem = @import("kernel").lib.mem;
const cpu = @import("kernel").drivers.cpu;
const Process = @import("kernel").lib.Process;
const kernel = @import("kernel");
const log = std.log.scoped(.smp);

pub export var SMP_REQUEST: limine.SmpRequest = .{ .flags = 1 };

pub const CPU = struct {
    ready: bool,
    cur_proc: ?*Process,
};

pub fn init() void {
    for (0..SMP_REQUEST.response.?.cpu_count) |i| {
        const info = SMP_REQUEST.response.?.cpus()[i];
        const _info = mem.kheap.allocator().create(CPU) catch @panic("OOM");
        _info.* = CPU{
            .ready = false,
            .cur_proc = null,
        };
        info.extra_argument = @intFromPtr(_info);
        if (info.lapic_id == SMP_REQUEST.response.?.bsp_lapic_id) {
            cpu.set_kgs(info.extra_argument);
        } else {
            info.goto_address = kernel.ap_init;
            while (!@atomicLoad(bool, &_info.ready, .acquire)) {}
        }
    }
    log.info("All application processors booted up and ready", .{});
}

/// Gets CPU info of {idx} cpu
/// If idx is null, gets current cpu info
pub fn cpu_info(idx: ?usize) *CPU {
    if (idx) |i| return @ptrFromInt(SMP_REQUEST.response.?.cpus_ptr[i].extra_argument);
    return @ptrFromInt(cpu.get_kgs());
}