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
        .{ .task = &lapic.Task },
        .{ .task = &timers.tsc.Task, .accept_failure = true },
    },
};

const HandlerCtx = struct {
    callback: timers.CallbackFn,
    ctx: ?*anyopaque,
    idt_handler: *idt.HandlerData, // Passed so handler can free ctx
};

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;
const TIMER_OFF = 0x320;

const IA32_TSC_DEADLINE = 0x6E0;

const CALIBRATION_NS = 1_000_000; // 1 ms

var hertz: usize = undefined;

fn init() kernel.Task.Ret {
    if (timers.timer != null) return .skipped;

    const tsc_deadline = cpu.cpuid(0x1, 0).ecx & (1 << 24);
    if (tsc_deadline > 0 and timers.tsc.Task.ret == .success) {
        // We are actually able to use TSC Deadline
        log.debug("Using TSC deadine mode", .{});
        vtable.callback = tsc_deadline_callback;
    } else {
        log.debug("Using normal oneshot mode", .{});
        vtable.callback = oneshot_callback;

        lapic.write_reg(INITIAL_COUNT_OFF, std.math.maxInt(u32));
        timers.sleep(CALIBRATION_NS);
        const cur_count = lapic.read_reg(CUR_COUNT_OFF);
        lapic.write_reg(INITIAL_COUNT_OFF, 0);
        hertz = (std.math.maxInt(u32) - cur_count) * (1_000_000_000 / CALIBRATION_NS);
    }

    timers.timer = &vtable;
    return .success;
}

fn callback_common(_ctx: ?*anyopaque, callback: timers.CallbackFn) u8 {
    const handler = idt.allocate_handler(null);
    handler.handler = timer_handler;
    const ctx = mem.kheap.allocator().create(HandlerCtx) catch @panic("OOM");
    ctx.* = .{
        .callback = callback,
        .ctx = _ctx,
        .idt_handler = handler,
    };
    handler.ctx = ctx;
    return idt.handler2vec(handler);
}

fn tsc_deadline_callback(ns: usize, ctx: ?*anyopaque, callback: timers.CallbackFn) void {
    lapic.write_reg(TIMER_OFF, @as(u32, @intCast(callback_common(ctx, callback))) | (0b10 << 17));
    const ticks = (ns * timers.tsc.hertz) / 1_000_000_000;
    cpu.wrmsr(IA32_TSC_DEADLINE, timers.tsc.rdtsc() + ticks);
}

fn oneshot_callback(ns: usize, ctx: ?*anyopaque, callback: timers.CallbackFn) void {
    lapic.write_reg(TIMER_OFF, @as(u32, @intCast(callback_common(ctx, callback))));
    const ticks = (ns * hertz) / 1_000_000_000;
    lapic.write_reg(INITIAL_COUNT_OFF, ticks);
}

fn timer_handler(_ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const ctx: *HandlerCtx = @ptrCast(@alignCast(_ctx));
    const ret = ctx.callback(ctx.ctx, status);
    kernel.drivers.intctrl.eoi(0);
    ctx.idt_handler.reserved = false;
    mem.kheap.allocator().destroy(ctx);
    return ret;
}
