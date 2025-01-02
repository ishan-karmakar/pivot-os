const kernel = @import("kernel");
const lapic = kernel.drivers.lapic;
const timers = kernel.drivers.timers;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const std = @import("std");
const log = std.log.scoped(.lapic);

pub const vtable = timers.TimerVTable{
    .callback = callback,
    .deinit = deinit,
};

pub var Task = kernel.Task{
    .name = "Local APIC Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &lapic.Task, .accept_failure = true },
    },
};

const INITIAL_COUNT_OFF = 0x380;
const CONFIG_OFF = 0x3E0;
const CUR_COUNT_OFF = 0x390;

const CALIBRATION_NS = 1_000_000; // 1 ms

var handler: timers.CallbackFn = undefined;
var idt_handler: *idt.HandlerData = undefined;
var ticks_per_ns: f64 = undefined;

fn init() bool {
    if (!lapic.Task.ret.?) return false;

    idt_handler = idt.allocate_handler(null);
    idt_handler.handler = timer_handler;

    lapic.write_reg(CONFIG_OFF, 0b10); // Divide by 8
    lapic.write_reg(INITIAL_COUNT_OFF, std.math.maxInt(u32));
    timers.sleep(CALIBRATION_NS);

    const cur_ticks = lapic.read_reg(CUR_COUNT_OFF);
    lapic.write_reg(INITIAL_COUNT_OFF, 0);
    const ticks_elapsed = std.math.maxInt(u32) - cur_ticks;
    ticks_per_ns = @floatFromInt(ticks_elapsed);
    ticks_per_ns /= CALIBRATION_NS;

    return true;
}

fn callback(ns: usize, ctx: ?*anyopaque, _handler: timers.CallbackFn) void {
    _handler = handler;
    idt_handler.ctx = ctx;

    lapic.write_reg(INITIAL_COUNT_OFF, @intFromFloat(ns * ticks_per_ns));
}

fn deinit() void {}

fn timer_handler(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const ret = handler(ctx, status);
    // LAPIC has no IRQ
    kernel.drivers.intctrl.controller.eoi(0);
    return ret;
}
