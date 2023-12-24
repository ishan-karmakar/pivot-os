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