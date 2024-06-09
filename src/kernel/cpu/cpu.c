#include <stdarg.h>
#include <cpu/cpu.h>
#include <cpu/idt.h>

void syscall(size_t id, size_t argc, ...) {
    va_list args;
    va_start(args, argc);
    uint64_t regs[6] = { 0, 0, 0, 0, 0, 0 };
    for (size_t i = 0; i < argc; i++)
        regs[i] = va_arg(args, uint64_t);

    asm (
        "mov %0, %%rax\n"
        "mov %1, %%rdi\n"
        "mov %2, %%rsi\n"
        "mov %3, %%rdx\n"
        "mov %4, %%r10\n"
        "mov %5, %%r8\n"
        "mov %6, %%r9\n"
        "int %7"
        : : "g" (id),
            "g" (regs[0]),
            "g" (regs[1]),
            "g" (regs[2]),
            "g" (regs[3]),
            "g" (regs[4]),
            "g" (regs[5]),
            "n" (SYSCALL_IDT_ENTRY)
            : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9");
    va_end(args);
}
