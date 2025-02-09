const std = @import("std");
const kernel = @import("kernel");
const cpu = kernel.drivers.cpu;
const log = std.log.scoped(.idt);

const ISR = *const fn () callconv(.Naked) void;

const Entry = packed struct {
    off0: u16,
    seg: u16 = 0x8,
    ist: u8 = 0,
    flags: u8,
    off1: u16,
    off2: u32,
    rsv: u32 = 0,
};

const IDTR = packed struct {
    size: u16,
    addr: usize,
};

var idtr = IDTR{
    .size = 256 * @sizeOf(Entry) - 1,
    .addr = 0,
};

var rawTable: [256]Entry = .{.{
    .off0 = 0,
    .off1 = 0,
    .off2 = 0,
    .flags = 0,
}} ** 256;

pub const HandlerData = struct {
    handler: ?*const fn (?*anyopaque, *cpu.Status) *const cpu.Status = null,
    ctx: ?*anyopaque = null,
    reserved: bool = false,
};

var handlerTable: [256 - 0x20]HandlerData = .{.{}} ** (256 - 0x20);

pub var Task = kernel.Task{
    .name = "IDT",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.gdt.StaticTask },
    },
};

pub var TaskAP = kernel.Task{
    .name = "IDT (AP)",
    .init = init_ap,
    .dependencies = &.{
        .{ .task = &kernel.drivers.gdt.DynamicTaskAP },
    },
};

fn init() kernel.Task.Ret {
    set_ent(0, create_exc_isr(0, false, "exception_handler"));
    set_ent(1, create_exc_isr(1, false, "exception_handler"));
    set_ent(2, create_exc_isr(2, false, "exception_handler"));
    set_ent(3, create_exc_isr(3, false, "exception_handler"));
    set_ent(4, create_exc_isr(4, false, "exception_handler"));
    set_ent(5, create_exc_isr(5, false, "exception_handler"));
    set_ent(6, create_exc_isr(6, false, "exception_handler"));
    set_ent(7, create_exc_isr(7, false, "exception_handler"));
    set_ent(8, create_exc_isr(8, true, "exception_handler"));
    set_ent(10, create_exc_isr(10, true, "exception_handler"));
    set_ent(11, create_exc_isr(11, true, "exception_handler"));
    set_ent(12, create_exc_isr(12, true, "exception_handler"));
    set_ent(13, create_exc_isr(13, true, "exception_handler"));
    set_ent(14, create_exc_isr(14, true, "pf_handler"));
    set_ent(16, create_exc_isr(16, false, "exception_handler"));
    set_ent(17, create_exc_isr(17, true, "exception_handler"));
    set_ent(18, create_exc_isr(18, false, "exception_handler"));
    set_ent(19, create_exc_isr(19, false, "exception_handler"));
    set_ent(20, create_exc_isr(20, false, "exception_handler"));
    set_ent(21, create_exc_isr(21, false, "exception_handler"));
    set_ent(22, create_exc_isr(22, false, "exception_handler"));
    set_ent(23, create_exc_isr(23, false, "exception_handler"));
    set_ent(24, create_exc_isr(24, false, "exception_handler"));
    set_ent(25, create_exc_isr(25, false, "exception_handler"));
    set_ent(26, create_exc_isr(26, false, "exception_handler"));
    set_ent(27, create_exc_isr(27, false, "exception_handler"));
    set_ent(28, create_exc_isr(28, false, "exception_handler"));
    set_ent(29, create_exc_isr(29, true, "exception_handler"));
    set_ent(30, create_exc_isr(30, true, "exception_handler"));

    inline for (0x20..256) |vec| set_ent(@intCast(vec), create_irq(vec));

    idtr.addr = @intFromPtr(&rawTable);
    lidt();
    return .success;
}

fn init_ap() kernel.Task.Ret {
    lidt();
    return .success;
}

fn set_ent(vec: u8, comptime handler: ISR) void {
    const ent = &rawTable[vec];
    const ptr = @intFromPtr(handler);
    ent.off0 = @truncate(ptr);
    ent.off1 = @truncate(ptr >> 16);
    ent.off2 = @truncate(ptr >> 32);
    ent.flags = 0x8E;
}

pub fn vec2handler(vec: u8) *HandlerData {
    if (vec < 0x20) @panic("Cannot get exception handler");
    return &handlerTable[vec - 0x20];
}

pub inline fn handler2vec(h: *const HandlerData) u8 {
    return @intCast((@intFromPtr(h) - @intFromPtr(&handlerTable[0])) / @sizeOf(HandlerData) + 0x20);
}

pub fn allocate_handler(pref: ?u8) *HandlerData {
    if (pref) |p| {
        const handler = vec2handler(p);
        if (!handler.reserved) {
            handler.reserved = true;
            return handler;
        }
    }
    for (&handlerTable) |*h| {
        if (!h.reserved) {
            h.reserved = true;
            return h;
        }
    }
    @panic("Out of interrupt handlers!!!");
}

fn create_exc_isr(comptime num: usize, comptime ec: bool, comptime fn_name: []const u8) ISR {
    return struct {
        fn handler() callconv(.Naked) void {
            if (comptime ec) {
                asm volatile ("pop %%rsi");
            } else asm volatile ("xor %%rsi, %%rsi");

            asm volatile (
                \\cli
                \\mov %[int_num], %%rdi
                \\mov %%rsp, %%rdx
                :
                : [int_num] "i" (num),
            );
            asm volatile ("jmp " ++ fn_name);
        }
    }.handler;
}

fn create_irq(comptime vec: usize) ISR {
    return struct {
        fn handler() callconv(.Naked) void {
            asm volatile (
                \\cli
                \\push %%rax
                \\push %%rbx
                \\push %%rcx
                \\push %%rdx
                \\push %%rbp
                \\push %%rsi
                \\push %%rdi
                \\push %%r8
                \\push %%r9
                \\push %%r10
                \\push %%r11
                \\push %%r12
                \\push %%r13
                \\push %%r14
                \\push %%r15
                \\
                \\mov %%rsp, %%rdi
                \\mov %[vec], %%rsi
                \\call irq_handler
                \\mov %%rax, %%rsp
                \\pop %%r15
                \\pop %%r14
                \\pop %%r13
                \\pop %%r12
                \\pop %%r11
                \\pop %%r10
                \\pop %%r9
                \\pop %%r8
                \\pop %%rdi
                \\pop %%rsi
                \\pop %%rbp
                \\pop %%rdx
                \\pop %%rcx
                \\pop %%rbx
                \\pop %%rax
                \\iretq
                :
                : [vec] "i" (vec),
            );
        }
    }.handler;
}

fn log_status(status: *const cpu.IRETStatus) void {
    log.debug("RIP: 0x{x}", .{status.rip});
    log.debug("CS: {}", .{status.cs});
    log.debug("RFLAGS: 0x{x}", .{status.rflags});
    log.debug("RSP: 0x{x}", .{status.rsp});
    log.debug("SS: {}", .{status.ss});
}

export fn irq_handler(status: *cpu.Status, vec: usize) *const cpu.Status {
    const handler = vec2handler(@intCast(vec));
    if (!handler.reserved) return status;
    if (handler.handler) |h| return h(handler.ctx, status);
    return status;
}

export fn pf_handler(_: usize, ec: usize, status: *const cpu.IRETStatus) noreturn {
    log.err("Encountered #PF with EC {}", .{ec});
    log.debug("CR2: 0x{x}", .{asm volatile ("mov %%cr2, %[result]"
        : [result] "=r" (-> u64),
    )});
    log_status(status);
    @panic("Panicking...");
}

export fn exception_handler(num: usize, ec: usize, status: *const cpu.IRETStatus) noreturn {
    log.err("Encountered exception {} with EC {}", .{ num, ec });
    log_status(status);
    @panic("Panicking...");
}

pub fn lidt() void {
    asm volatile ("lidt (%[idtr])"
        :
        : [idtr] "r" (&idtr),
    );
}
