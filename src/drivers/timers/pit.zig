const kernel = @import("kernel");
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

pub const vtable = timers.VTable{
    .callback = callback,
    .time = null,
};

pub var Task = kernel.Task{
    .name = "PIT",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.idt.Task },
        .{ .task = &kernel.drivers.intctrl.Task },
    },
};

const THandlerCtx = struct {
    callback: timers.CallbackFn,
    irq: usize,
};
var thandler_ctx: THandlerCtx = undefined;
var idt_handler: *idt.HandlerData = undefined;

fn init() kernel.Task.Ret {
    if (timers.timer != null) return .skipped;
    idt_handler = idt.allocate_handler(intctrl.pref_vec(IRQ));
    thandler_ctx.irq = intctrl.map(idt.handler2vec(idt_handler), IRQ) catch {
        idt_handler.reserved = false;
        return .failed;
    };
    idt_handler.handler = timer_handler;

    var triggered: bool = false;
    callback(&vtable, 1_000_000, &triggered, disable_pit_callback);
    while (!@atomicLoad(bool, @as(*const volatile bool, @ptrCast(&triggered)), .acquire)) asm volatile ("pause");
    timers.timer = &vtable;
    return .success;
}

fn disable_pit_callback(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    @as(*bool, @alignCast(@ptrCast(ctx))).* = true;
    return status;
}

fn callback(_: *timers.VTable, ns: usize, ctx: ?*anyopaque, handler: timers.CallbackFn) void {
    const ticks = (ns * HZ) / 1_000_000_000;
    thandler_ctx.callback = handler;
    idt_handler.ctx = ctx;
    serial.out(CMD_REG, @as(u8, 0x30));
    serial.out(DATA_REG, @as(u8, @truncate(ticks)));
    serial.out(DATA_REG, @as(u8, @truncate(ticks >> 8)));
    intctrl.controller().mask(thandler_ctx.irq, false);
}

fn timer_handler(callback_ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    const ret = thandler_ctx.callback(callback_ctx, status);
    // No need to mask because we are only doing oneshot ints
    intctrl.controller().eoi(thandler_ctx.irq);
    return ret;
}
