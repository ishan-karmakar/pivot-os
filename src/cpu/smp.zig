const kernel = @import("root");
const limine = @import("limine");
const std = @import("std");
const mem = kernel.mem;
const cpu = kernel.cpu;
const log = std.log.scoped(.smp);

pub export var SMP_REQUEST = limine.limine_mp_request{
    .id = kernel.LIMINE_REQUEST_ID(0x95a67b819a1b857e, 0xa0b61b723b6a73e0),
    .flags = limine.LIMINE_MP_REQUEST_X86_64_X2APIC,
};

pub const CPU = struct {
    id: u32,
    cur_proc: ?*kernel.cpu.scheduler.Thread,
    lapic_handler: *kernel.cpu.idt.HandlerData,

    lapic_initialized: bool = false,
};

var bsp_cpu_info = CPU{
    .id = 0,
    .cur_proc = null,
    .lapic_handler = undefined,
};
var initialized = false;

pub fn init_bsp() !void {
    const response: *limine.limine_mp_response = SMP_REQUEST.response orelse
        return kernel.lib.logger.failed_initialization(log, "SMP (BSP)", error.SMPUnavailable);

    bsp_cpu_info.id = SMP_REQUEST.response.*.bsp_lapic_id;
    response.cpus[response.bsp_lapic_id].*.extra_argument = @intFromPtr(&bsp_cpu_info);
    kernel.cpu.set_kgs(@intFromPtr(&bsp_cpu_info));
    kernel.lib.logger.successfully_initialized(log, "SMP (BSP)");
}

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "SMP");
    init_bsp() catch |err|
        return kernel.lib.logger.failed_initialization(log, "SMP", err);
    kernel.mem.init_kheap() catch |err|
        return kernel.lib.logger.failed_initialization(log, "SMP", err);
    kernel.cpu.gdt.init_dynamic() catch |err|
        return kernel.lib.logger.failed_initialization(log, "SMP", err);
    kernel.cpu.lapic.init_bsp() catch |err|
        return kernel.lib.logger.failed_initialization(log, "SMP", err);

    const response: *limine.limine_mp_response = SMP_REQUEST.response orelse
        return kernel.lib.logger.failed_initialization(log, "SMP", error.SMPUnavailable);
    for (0..response.cpu_count) |i| {
        const info: *limine.limine_mp_info = response.cpus[i];
        if (info.lapic_id == response.bsp_lapic_id)
            continue;
        const _info = mem.kheap.allocator().create(CPU) catch |err|
            return kernel.lib.logger.failed_initialization(log, "SMP", err);
        _info.* = CPU{
            .id = info.lapic_id,
            .cur_proc = null,
            .lapic_handler = undefined,
        };
        info.extra_argument = @intFromPtr(_info);
        @atomicStore(usize, @as(*usize, @ptrCast(&info.goto_address)), @intFromPtr(&ap_init), .unordered);
    }
    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "SMP");
}

fn ap_init(info: *limine.limine_mp_info) callconv(.c) noreturn {
    kernel.cpu.set_kgs(info.extra_argument);
    kernel.cpu.idt.init_ap();
    kernel.mem.init_kmapper_ap();
    kernel.cpu.gdt.init_ap();
    kernel.cpu.lapic.init_ap();
    asm volatile ("sti");
    while (true) asm volatile ("hlt");
}

/// Gets CPU info of {idx} cpu
/// If idx is null, gets current cpu info
pub fn cpu_info(idx: ?usize) *CPU {
    if (idx) |i| return @ptrFromInt(SMP_REQUEST.response.*.cpus[i].*.extra_argument);
    return @ptrFromInt(cpu.get_kgs());
}

pub inline fn cpu_count() usize {
    return SMP_REQUEST.response.*.cpu_count;
}
