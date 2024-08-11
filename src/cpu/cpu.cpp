#include <cpu/cpu.hpp>
#include <util/logger.hpp>
#include <cpuid.h>
using namespace cpu;

void enable_sse();
void enable_avx();
void enable_smap(uint32_t);
void enable_smep(uint32_t);
void enable_ne();

constexpr uint32_t IA32_EFER = 0xC0000080;

void cpu::init() {
    // Enable SSE
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    if (edx & (1 << 25)) {
        enable_sse();
        logger::verbose("CPU[SSE]", "%sSSE%s%s%s%s%s is available",
            ecx & (1 << 9) ? "SSSE3, " : "",
            edx & (1 << 26) ? "2" : "",
            ecx & 1 ? ", 3" : "",
            ecx & (1 << 19) ? ", 4.1" : "",
            ecx & (1 << 20) ? ", 4.2" : "",
            ecx & (1 << 6) ? ", 4A" : ""
        );

        if (ecx & (1 << 28)) // UNTESTED
            enable_avx();
    } else
        logger::panic("CPU[INIT]", "SSE is not available");

    // UNTESTED
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    enable_smap(ebx);
    enable_smep(ebx);
}

[[gnu::always_inline]]
inline void enable_sse() {
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
    logger::verbose("CPU[SSE]", "Enabled");
}

[[gnu::always_inline]]
inline void enable_avx() {
    asm volatile (
        "mov $0, %%rcx;"
        "xgetbv;"
        "or $7, %%rax;"
        "xsetbv;"
        : : : "rdx", "rcx", "rax"
    );
    logger::verbose("CPU[AVX]", "Enabled");
}

[[gnu::always_inline]]
inline void enable_smap(uint32_t ebx) {
    if (ebx & (1 << 7)) {
        asm volatile (
            "mov %%cr4, %%rax;"
            "or $0x100000, %%rax;"
            "mov %%rax, %%cr4;"
            : : : "rax"
        );
        logger::verbose("CPU[SMAP]", "Enabled");
    }
}

[[gnu::always_inline]]
inline void enable_smep(uint32_t ebx) {
    if (ebx & (1 << 20)) {
        asm volatile (
            "mov %%cr4, %%rax;"
            "or $0x200000, %%rax;"
            "mov %%rax, %%cr4;"
            : : : "rax"
        );
        logger::verbose("CPU[SMEP]", "Enabled");
    }
}

[[gnu::always_inline]]
inline void enable_ne() {
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | (1 << 11));
    logger::verbose("CPU[NE]", "Enabled");
}
