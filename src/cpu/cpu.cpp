#include <cpu/cpu.hpp>
#include <util/logger.hpp>
#include <cpuid.h>
using namespace cpu;

void enable_sse();
void enable_avx();

void cpu::init() {
    // Enable SSE
    uint32_t ignored, ecx, edx;
    __get_cpuid(1, &ignored, &ignored, &ecx, &edx);

    if (edx & (1 << 25)) {
        enable_sse();
        log(VERBOSE, "CPU", "%sSSE%s%s%s%s%s is available",
            ecx & (1 << 9) ? "SSSE3, " : "",
            edx & (1 << 26) ? "2" : "",
            ecx & 1 ? ", 3" : "",
            ecx & (1 << 19) ? ", 4.1" : "",
            ecx & (1 << 20) ? ", 4.2" : "",
            ecx & (1 << 6) ? ", 4A" : ""
        );

        if (ecx & (1 << 28)) {
            enable_avx();
        }
    } else
        log(VERBOSE, "CPU", "SSE is not available");
}

void enable_sse() {
    asm volatile (
        "mov %%cr0, %%rax;"
        "and $0xFFFFFFFFFFFFFFFB, %%rax;"
        "or $2, %%rax;"
        "mov %%rax, %%cr0;"
        "mov %%cr4, %%rax;"
        "or $0x600, %%rax;"
        "mov %%rax, %%cr4;"
        : : : "rax"
    );
    log(VERBOSE, "CPU", "Enabled SSE");
}

void enable_avx() {}
