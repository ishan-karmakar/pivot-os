const kernel = @import("root");
const timers = kernel.drivers.timers;
const intctrl = kernel.drivers.intctrl;
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;
const serial = kernel.drivers.serial;
const math = @import("std").math;
const log = @import("std").log.scoped(.pit);

const CMD_REG = 0x43;
const DATA_REG = 0x40;
const IRQ = 0;
const HZ = 1193182;

pub var TimerTask = kernel.Task{
    .name = "PIT Timer",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
        .{ .task = &kernel.drivers.intctrl.Task },
    },
};

pub const TimerVTable = timers.TimerVTable{
    .requires_calibration = false,
    .callback = callback,
};

const THandlerCtx = struct {
    callback: timers.CallbackFn,
    irq: usize,
};
var thandler_ctx: THandlerCtx = undefined;
var idt_handler: *idt.HandlerData = undefined;

fn init() kernel.Task.Ret {
    idt_handler = idt.allocate_handler(intctrl.pref_vec(IRQ));
    thandler_ctx.irq = intctrl.map(idt.handler2vec(idt_handler), IRQ) catch {
        idt_handler.reserved = false;
        return .failed;
    };
    idt_handler.handler = timer_handler;

    var triggered: bool = false;
    callback(1_000_000, &triggered, disable_pit_callback);
    while (!@atomicLoad(bool, @as(*const volatile bool, @ptrCast(&triggered)), .acquire)) asm volatile ("pause");
    return .success;
}

fn disable_pit_callback(ctx: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    @as(*bool, @ptrCast(@alignCast(ctx))).* = true;
    return status;
}

fn callback(ns: usize, ctx: ?*anyopaque, handler: timers.CallbackFn) void {
    const ticks = (ns * HZ) / 1_000_000_000;
    thandler_ctx.callback = handler;
    idt_handler.ctx = ctx;
    serial.out(CMD_REG, @as(u8, 0x30));
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.mask(thandler_ctx.irq, false);
}

fn timer_handler(callback_ctx: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    const ret = thandler_ctx.callback(callback_ctx, status);
    // No need to mask because we are only doing oneshot ints
    intctrl.eoi(thandler_ctx.irq);
    return ret;
}
