const kernel = @import("kernel");
const std = @import("std");
const log = std.log.scoped(.syscall);
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;

pub var Task = kernel.Task{
    .name = "Syscalls",
    .init = init,
    .dependencies = &.{
        .{ .task = &idt.Task },
    },
};

pub const SyscallHandler = *const fn (*cpu.Status) *const cpu.Status;

pub const SYSCALLS = enum(c_int) {
    EXIT,
    SLEEP,
};

var syscalls: [@typeInfo(SYSCALLS).Enum.fields.len]SyscallHandler = undefined;

fn init() kernel.Task.Ret {
    const handler = idt.allocate_handler(0x80);
    if (idt.handler2vec(handler) != 0x80) return .failed;

    handler.handler = syscall_handler;

    return .success;
}

pub fn register_syscall(idx: SYSCALLS, handler: SyscallHandler) void {
    syscalls[@intCast(@intFromEnum(idx))] = handler;
}

fn syscall_handler(_: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    return syscalls[status.rdi](status);
}

pub fn syscall(_: SYSCALLS, ...) callconv(.C) usize {
    // Wrapper over SYS V calling convention
    return asm volatile ("int $0x80"
        : [result] "=rax" (-> usize),
    );
}
