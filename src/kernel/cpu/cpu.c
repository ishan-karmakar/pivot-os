#include <stdarg.h>
#include <cpu/cpu.h>

uint64_t rdmsr(uint32_t address) {
    uint32_t low = 0, high = 0;
    asm volatile (
        "mov %2, %%ecx;"
        "rdmsr;"
        : "=a" (low), "=d" (high)
        : "g" (address)
    );
    return (uint64_t) low | ((uint64_t) high << 32);
}

void wrmsr(uint32_t address, uint64_t value) {
    asm volatile ("wrmsr" : : "a" ((uint32_t) value), "d" (value >> 32), "c" (address));
}

void load_cr3(uintptr_t addr) {
    asm volatile ("mov %0, %%cr3" :: "r" (addr) : "memory");
}

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
        "int $0x80"
        : : "r" (id),
            "r" (regs[0]),
            "r" (regs[1]),
            "r" (regs[2]),
            "r" (regs[3]),
            "r" (regs[4]),
            "r" (regs[5])
            : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9");
    va_end(args);
}
