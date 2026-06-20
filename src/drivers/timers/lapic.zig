const kernel = @import("root");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;
const mem = kernel.lib.mem;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const smp = kernel.lib.smp;
const std = @import("std");
const log = std.log.scoped(.lapic_timer);

const tsc = @import("tsc.zig");

var CLOCKEVENT = timers.ClockEvent{
    .features = .{ .per_cpu = true },
    .oneshot = undefined,
    .rating = 125,
};

const HandlerCtx = struct {
    callback: timers.CallbackFn,
    ctx: ?*anyopaque,
};

const INITIAL_COUNT_OFF = 0x380;
const CUR_COUNT_OFF = 0x390;
const TIMER_OFF = 0x320;

const IA32_TSC_DEADLINE = 0x6E0;

const CALIBRATION_NS = 1_000_000; // 1 ms

var hertz: usize = undefined;
var initialized = false;

pub fn init() !void {
    if (initialized)
        return kernel.lib.logger.already_initialized(log, "LAPIC Timer");
    kernel.drivers.lapic.init_bsp() catch |err|
        return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);
    mem.init_kheap() catch |err|
        return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);
    kernel.drivers.intctrl.init() catch |err|
        return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);
    smp.init() catch |err|
        return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);

    const tsc_deadline = cpu.cpuid(0x1, 0).ecx & (1 << 24);
    if (tsc_deadline > 0 and tsc.is_supported()) {
        // We are actually able to use TSC Deadline
        log.debug("Using TSC deadine mode", .{});
        CLOCKEVENT.oneshot = tsc_deadline_callback;
    } else {
        log.debug("Using normal oneshot mode", .{});
        CLOCKEVENT.oneshot = oneshot_callback;

        lapic.write_reg(INITIAL_COUNT_OFF, std.math.maxInt(u32));
        timers.sleep(CALIBRATION_NS) catch |err|
            return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);
        const cur_count = lapic.read_reg(CUR_COUNT_OFF);
        lapic.write_reg(INITIAL_COUNT_OFF, 0);
        hertz = (std.math.maxInt(u32) - cur_count) * (1_000_000_000 / CALIBRATION_NS);
    }

    for (0..smp.cpu_count()) |i| {
        const cpu_info = smp.cpu_info(i);
        cpu_info.lapic_handler = idt.allocate_handler(null);
        cpu_info.lapic_handler.handler = timer_handler;
        cpu_info.lapic_handler.ctx = mem.kheap.allocator().create(HandlerCtx) catch |err|
            return kernel.lib.logger.failed_initialization(log, "LAPIC Timer", err);
    }

    initialized = true;
    kernel.lib.logger.successfully_initialized(log, "LAPIC Timer");
}

fn callback_common(_ctx: ?*anyopaque, callback: timers.CallbackFn) u8 {
    const handler = smp.cpu_info(null).lapic_handler;
    const ctx: *HandlerCtx = @ptrCast(@alignCast(handler.ctx));
    ctx.* = .{
        .callback = callback,
        .ctx = _ctx,
    };
    return idt.handler2vec(handler);
}

fn tsc_deadline_callback(ns: usize, ctx: ?*anyopaque, callback: timers.CallbackFn) void {
    lapic.write_reg(TIMER_OFF, @as(u32, @intCast(callback_common(ctx, callback))) | (0b10 << 17));
    cpu.wrmsr(IA32_TSC_DEADLINE, tsc.rdtsc() + tsc.ns2ticks(ns));
}

fn oneshot_callback(ns: usize, ctx: ?*anyopaque, callback: timers.CallbackFn) void {
    lapic.write_reg(TIMER_OFF, @as(u32, @intCast(callback_common(ctx, callback))));
    lapic.write_reg(INITIAL_COUNT_OFF, tsc.ns2ticks(ns));
}

fn timer_handler(_ctx: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    const ctx: *HandlerCtx = @ptrCast(@alignCast(_ctx));
    const ret = ctx.callback(ctx.ctx, status);
    kernel.drivers.intctrl.eoi(0);
    return ret;
}
