const kernel = @import("kernel");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;
const mem = kernel.lib.mem;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = std.log.scoped(.lapic_timer);

pub var vtable = timers.VTable{
    .callback = null,
    .time = null,
};

pub var Task = kernel.Task{
    .name = "Local APIC Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &lapic.Task, .accept_failure = true },
        .{ .task = &timers.tsc.Task, .accept_failure = true },
    },
};

const HandlerCtx = struct {
    callback: timers.CallbackFn,
    ctx: ?*anyopaque,
    idt_handler: *const idt.HandlerData, // Passed so handler can free ctx
};

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;
const TIMER_OFF = 0x320;

const IA32_TSC_DEADLINE = 0x6E0;

const CALIBRATION_NS = 1_000_000; // 1 ms

fn init() kernel.Task.Ret {
    if (lapic.Task.ret != .success) return .failed;

    const tsc_deadline = cpu.cpuid(0x1, 0).ecx & (1 << 24);
    if (tsc_deadline > 0 and timers.tsc.Task.ret == .success) {
        // We are actually able to use TSC Deadline
        log.debug("Using TSC deadine mode", .{});
        vtable.callback = tsc_deadline_callback;
        timers.timer = &vtable;
        return .success;
    }

    log.debug("Using normal oneshot mode", .{});
    vtable.callback = oneshot_callback;
    // Calibrate timer

    return .success;
}

fn tsc_deadline_callback(ns: usize, _ctx: ?*anyopaque, callback: timers.CallbackFn) void {
    const ticks = (ns * timers.tsc.hertz) / 1_000_000_000;
    const handler = idt.allocate_handler(null);
    handler.handler = timer_handler;
    const ctx = mem.kheap.allocator().create(HandlerCtx) catch @panic("OOM");
    ctx.* = .{
        .callback = callback,
        .ctx = _ctx,
        .idt_handler = handler,
    };
    handler.ctx = ctx;
    lapic.write_reg(TIMER_OFF, @as(u32, @intCast(idt.handler2vec(handler))) | (0b10 << 17));
    cpu.wrmsr(IA32_TSC_DEADLINE, timers.tsc.rdtsc() + ticks);
}

fn oneshot_callback(ns: usize, ctx: ?*anyopaque, _handler: timers.CallbackFn) void {
    _ = ns;
    _ = ctx;
    _ = _handler;
}

fn timer_handler(_ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const ctx: *HandlerCtx = @ptrCast(@alignCast(_ctx));
    const ret = ctx.callback(ctx.ctx, status);
    kernel.drivers.intctrl.eoi(0);
    mem.kheap.allocator().destroy(ctx);
    return ret;
}
