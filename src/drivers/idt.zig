const std = @import("std");
const cpu = @import("kernel").drivers.cpu;
const log = std.log.scoped(.idt);

const ISR = fn () callconv(.Naked) void;

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

pub var table: [256]Entry = .{.{
    .off0 = 0,
    .off1 = 0,
    .off2 = 0,
    .flags = 0,
}} ** 256;

const Handler = *const fn (?*anyopaque, *const cpu.Status) *const cpu.Status;
const HandlerInfo = struct {
    handler: ?Handler = null,
    ctx: ?*anyopaque = null,
    reserved: bool = false,
};
var handlers: [256 - 0x20]HandlerInfo = .{.{}} ** (256 - 0x20);

pub fn init() void {
    set_ent(0, create_exception_isr(0, false, "exception_handler"));
    set_ent(1, create_exception_isr(1, false, "exception_handler"));
    set_ent(2, create_exception_isr(2, false, "exception_handler"));
    set_ent(3, create_exception_isr(3, false, "exception_handler"));
    set_ent(4, create_exception_isr(4, false, "exception_handler"));
    set_ent(5, create_exception_isr(5, false, "exception_handler"));
    set_ent(6, create_exception_isr(6, false, "exception_handler"));
    set_ent(7, create_exception_isr(7, false, "exception_handler"));
    set_ent(8, create_exception_isr(8, true, "exception_handler"));
    set_ent(10, create_exception_isr(10, true, "exception_handler"));
    set_ent(11, create_exception_isr(11, true, "exception_handler"));
    set_ent(12, create_exception_isr(12, true, "exception_handler"));
    set_ent(13, create_exception_isr(13, true, "exception_handler"));
    set_ent(14, create_exception_isr(14, true, "pf_handler"));
    set_ent(16, create_exception_isr(16, false, "exception_handler"));
    set_ent(17, create_exception_isr(17, true, "exception_handler"));
    set_ent(18, create_exception_isr(18, false, "exception_handler"));
    set_ent(19, create_exception_isr(19, false, "exception_handler"));
    set_ent(20, create_exception_isr(20, false, "exception_handler"));
    set_ent(21, create_exception_isr(21, false, "exception_handler"));
    set_ent(22, create_exception_isr(22, false, "exception_handler"));
    set_ent(23, create_exception_isr(23, false, "exception_handler"));
    set_ent(24, create_exception_isr(24, false, "exception_handler"));
    set_ent(25, create_exception_isr(25, false, "exception_handler"));
    set_ent(26, create_exception_isr(26, false, "exception_handler"));
    set_ent(27, create_exception_isr(27, false, "exception_handler"));
    set_ent(28, create_exception_isr(28, false, "exception_handler"));
    set_ent(29, create_exception_isr(29, true, "exception_handler"));
    set_ent(30, create_exception_isr(30, true, "exception_handler"));

    inline for (0x20..256) |vec| set_ent(@truncate(vec), create_irq(vec - 0x20));
    reserve_vecs(0x80, 0x80); // Syscall int number

    idtr.addr = @intFromPtr(&table);
    lidt();
    log.info("Loaded interrupt descriptor table", .{});
}

fn set_ent(vec: u8, comptime handler: ISR) void {
    const ent = &table[vec];
    const ptr = @intFromPtr(&handler);
    ent.off0 = @truncate(ptr);
    ent.off1 = @truncate(ptr >> 16);
    ent.off2 = @truncate(ptr >> 32);
    ent.flags = 0x8E;
}

pub fn lidt() void {
    asm volatile ("lidt (%[idtr])"
        :
        : [idtr] "r" (&idtr),
    );
}

pub fn allocate(handler: Handler, ctx: ?*anyopaque) ?u8 {
    for (32.., &handlers) |i, *h| {
        if (h.reserved) continue;
        h.handler = handler;
        h.ctx = ctx;
        h.reserved = true;
        return @intCast(i);
    }
    return null;
}

pub fn allocate_vec(_vec: u8, handler: Handler, ctx: ?*anyopaque) ?u8 {
    if (_vec < 32) return null;
    const vec = _vec - 32;
    if (handlers[vec].reserved) return allocate(handler, ctx);
    handlers[vec].reserved = true;
    handlers[vec].handler = handler;
    handlers[vec].ctx = ctx;
    return _vec;
}

pub fn set_ctx(vec: u8, ctx: ?*anyopaque) void {
    if (vec < 32) return;
    handlers[vec - 32].ctx = ctx;
}

/// Reserves IRQs in the range [start, end)
pub fn reserve_vecs(start: usize, end: usize) void {
    if (start < 32) return;
    for (start - 32..end - 32) |i| handlers[i].reserved = true;
}

pub inline fn free_vec(vec: usize) void {
    free_vecs(vec, vec + 1);
}

pub fn free_vecs(start: usize, end: usize) void {
    if (start < 32) return;
    for (start - 32..end - 32) |i| {
        handlers[i].reserved = false;
        handlers[i].handler = null;
    }
}

export fn irq_handler(status: *const cpu.Status, irq: usize) *const cpu.Status {
    if (handlers[irq].handler) |h| return h(handlers[irq].ctx, status);
    return status;
}

export fn exception_handler(int_num: usize, ec: usize, status: *const cpu.IRETStatus) noreturn {
    log.err("Encountered exception {} with EC {}", .{ int_num, ec });
    log_status(status);
    @panic("Panicking...");
}

export fn pf_handler(_: usize, ec: usize, status: *const cpu.IRETStatus) noreturn {
    log.err("Encountered #PF with EC {}", .{ec});
    log.debug("CR2: 0x{x}", .{asm volatile ("mov %%cr2, %[result]"
        : [result] "=r" (-> u64),
    )});
    log_status(status);
    @panic("Panicking...");
}

fn log_status(status: *const cpu.IRETStatus) void {
    log.debug("RIP: 0x{x}", .{status.rip});
    log.debug("CS: {}", .{status.cs});
    log.debug("RFLAGS: 0x{x}", .{status.rflags});
    log.debug("RSP: 0x{x}", .{status.rsp});
    log.debug("SS: {}", .{status.ss});
}

fn create_exception_isr(comptime int_num: usize, comptime ec: bool, comptime fn_name: []const u8) ISR {
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
                : [int_num] "i" (int_num),
            );
            asm volatile ("jmp " ++ fn_name);
        }
    }.handler;
}

pub fn create_irq(comptime int_num: usize) ISR {
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
                \\mov %[int_num], %%rsi
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
                : [int_num] "i" (int_num),
            );
            // TODO: Handle CR3 change
        }
    }.handler;
}
