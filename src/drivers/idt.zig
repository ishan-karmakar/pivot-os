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

var table: [256]Entry = .{.{
    .off0 = 0,
    .off1 = 0,
    .off2 = 0,
    .flags = 0,
}} ** 256;

pub fn init() void {
    set_idt_ent(&table[0], create_exception_isr(0, false));
    set_idt_ent(&table[1], create_exception_isr(1, false));
    set_idt_ent(&table[2], create_exception_isr(2, false));
    set_idt_ent(&table[3], create_exception_isr(3, false));
    set_idt_ent(&table[4], create_exception_isr(4, false));
    set_idt_ent(&table[5], create_exception_isr(5, false));
    set_idt_ent(&table[6], create_exception_isr(6, false));
    set_idt_ent(&table[7], create_exception_isr(7, false));
    set_idt_ent(&table[8], create_exception_isr(8, true));
    set_idt_ent(&table[10], create_exception_isr(10, true));
    set_idt_ent(&table[11], create_exception_isr(11, true));
    set_idt_ent(&table[12], create_exception_isr(12, true));
    set_idt_ent(&table[13], create_exception_isr(13, true));
    set_idt_ent(&table[14], pf_handler);
    set_idt_ent(&table[16], create_exception_isr(16, false));
    set_idt_ent(&table[17], create_exception_isr(17, true));
    set_idt_ent(&table[18], create_exception_isr(18, false));
    set_idt_ent(&table[19], create_exception_isr(19, false));
    set_idt_ent(&table[20], create_exception_isr(20, false));
    set_idt_ent(&table[21], create_exception_isr(21, false));
    set_idt_ent(&table[22], create_exception_isr(22, false));
    set_idt_ent(&table[23], create_exception_isr(23, false));
    set_idt_ent(&table[24], create_exception_isr(24, false));
    set_idt_ent(&table[25], create_exception_isr(25, false));
    set_idt_ent(&table[26], create_exception_isr(26, false));
    set_idt_ent(&table[27], create_exception_isr(27, false));
    set_idt_ent(&table[28], create_exception_isr(28, false));
    set_idt_ent(&table[29], create_exception_isr(29, true));
    set_idt_ent(&table[30], create_exception_isr(30, true));

    idtr.addr = @intFromPtr(&table);
    lidt();
    log.info("Loaded interrupt descriptor table", .{});
}

fn set_idt_ent(ent: *Entry, comptime handler: ISR) void {
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

fn create_exception_isr(comptime int_num: usize, comptime ec: bool) ISR {
    return struct {
        fn handler() callconv(.Naked) void {
            if (comptime !ec) {
                asm volatile ("push $0");
            }

            asm volatile (
                \\mov %[int_num], %%rdi
                \\mov %%rsp, %%rsi
                \\jmp exception_handler
                :
                : [int_num] "i" (int_num),
            );
        }
    }.handler;
}

// fn create_fn_table() [256]fn () callconv(.Naked) void {
//     var t: [256]fn () callconv(.Naked) void = undefined;
//     t[0] = create_exception_handler(0, false);
//     t[1] = create_exception_handler(1, false);
//     t[2] = create_exception_handler(2, false);
//     t[3] = create_exception_handler(3, false);
//     t[4] = create_exception_handler(4, false);
//     t[5] = create_exception_handler(5, false);
//     t[6] = create_exception_handler(6, false);
//     t[7] = create_exception_handler(7, false);
//     t[8] = create_exception_handler(8, true);
//     t[10] = create_exception_handler(10, true);
//     t[11] = create_exception_handler(11, true);
//     t[12] = create_exception_handler(12, true);
//     t[13] = create_exception_handler(13, true);
//     t[14] = create_exception_handler(14, true);
//     t[16] = create_exception_handler(16, false);
//     t[17] = create_exception_handler(17, true);
//     t[18] = create_exception_handler(18, false);
//     t[19] = create_exception_handler(19, false);
//     t[20] = create_exception_handler(20, false);
//     t[21] = create_exception_handler(21, false);
//     t[22] = create_exception_handler(22, false);
//     t[23] = create_exception_handler(23, false);
//     t[24] = create_exception_handler(24, false);
//     t[25] = create_exception_handler(25, false);
//     t[26] = create_exception_handler(26, false);
//     t[27] = create_exception_handler(27, false);
//     t[28] = create_exception_handler(28, false);
//     t[29] = create_exception_handler(29, true);
//     t[30] = create_exception_handler(30, true);
//     return t;
// }

// Loads the function addresses from fn_table to IDT table at runtime
// fn load_idt_table() void {
//     for (0..256) |i| {
//         const ptr = @intFromPtr(&fn_table[i]);
//         table[i].off0 = ptr & 0xFFFF;
//         table[i].off1 = ptr & 0xFFFF;
//         table[i].off2 = ptr & 0xFFFFFFFF;
//         table[i].flags = 0x8E;
//     }
// }
