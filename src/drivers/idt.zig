const std = @import("std");
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

const ExceptionStatus = struct {
    ec: u64,
    rip: u64,
    cs: u64,
    rflags: u64,
    rsp: u64,
    ss: u64,
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

    idtr.addr = @intFromPtr(&table);
    lidt();
    log.info("Loaded interrupt descriptor table", .{});
}

pub fn set_ent(vec: u8, comptime handler: ISR) void {
    const ent = &table[vec];
    const ptr = @intFromPtr(&handler);
    ent.off0 = @truncate(ptr);
    ent.off1 = @truncate(ptr >> 16);
    ent.off2 = @truncate(ptr >> 32);
    ent.flags = 0x8E;
}

fn lidt() void {
    asm volatile ("lidt (%[idtr])"
        :
        : [idtr] "r" (&idtr),
    );
}

export fn exception_handler(int_num: usize, status: *const ExceptionStatus) noreturn {
    log.err("Encountered exception {} with EC {}", .{ int_num, status.ec });
    log_status(status);
    @panic("Panicking...");
}

export fn pf_handler(_: usize, status: *const ExceptionStatus) noreturn {
    log.err("Encountered #PF with EC {}", .{status.ec});
    log.debug("CR2: 0x{x}", .{asm volatile ("mov %%cr2, %[result]"
        : [result] "=r" (-> u64),
    )});
    log_status(status);
    @panic("Panicking...");
}

fn log_status(status: *const ExceptionStatus) void {
    log.debug("RIP: 0x{x}", .{status.rip});
    log.debug("CS: {}", .{status.cs});
    log.debug("RFLAGS: 0x{x}", .{status.rflags});
    log.debug("RSP: 0x{x}", .{status.rsp});
    log.debug("SS: {}", .{status.ss});
}

// export fn exception_common() callconv(.Naked) void {
//     asm volatile (
//         \\push %%rax
//         \\push %%rbx
//         \\push %%rcx
//         \\push %%rdx
//         \\push %%rbp
//         \\push %%rsi
//         \\push %%rdi
//         \\push %%r8
//         \\push %%r9
//         \\push %%r10
//         \\push %%r11
//         \\push %%r12
//         \\push %%r13
//         \\push %%r14
//         \\push %%r15
//         \\
//         \\mov %%rsp, %%rdi
//         \\jmp exception_handler
//     );
// }

fn create_exception_isr(comptime int_num: usize, comptime ec: bool, comptime fn_name: []const u8) ISR {
    return struct {
        fn handler() callconv(.Naked) void {
            if (comptime !ec) {
                asm volatile ("push $0");
            }

            asm volatile (
                \\mov %[int_num], %%rdi
                \\mov %%rsp, %%rsi
                :
                : [int_num] "i" (int_num),
            );
            asm volatile ("jmp " ++ fn_name);
        }
    }.handler;
}

pub fn create_irq(comptime needs_status: bool, comptime handler_name: []const u8) ISR {
    _ = needs_status;
    return struct {
        fn handler() callconv(.Naked) void {
            asm volatile ("cli");
            asm volatile ("call " ++ handler_name);
            asm volatile ("iretq");
        }
    }.handler;
}
