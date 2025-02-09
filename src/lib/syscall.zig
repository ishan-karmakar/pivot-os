const kernel = @import("kernel");
const log = @import("std").log.scoped(.syscall);
const idt = kernel.drivers.idt;
const cpu = kernel.drivers.cpu;

pub var Task = kernel.Task{
    .name = "Syscalls",
    .init = init,
    .dependencies = &.{
        .{ .task = &idt.Task },
    },
};

fn init() kernel.Task.Ret {
    const handler = idt.allocate_handler(0x80);
    if (idt.handler2vec(handler) != 0x80) return .failed;

    handler.handler = syscall_handler;

    return .success;
}

fn syscall_handler(_: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    log.info("Received syscall {}", .{status.rdi});
    return status;
}

pub fn syscall(_: usize, ...) callconv(.C) usize {
    return asm volatile ("int $0x80"
        : [result] "=rax" (-> usize),
    );
}
