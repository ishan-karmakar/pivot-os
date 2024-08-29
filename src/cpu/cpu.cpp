#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <cpuid.h>
using namespace cpu;

void enable_smap();
void enable_smep();
void detect_fpu();

constexpr uint32_t IA32_EFER = 0xC0000080;

std::function<void(void*)> cpu::fpu_save;
std::function<void(void*)> cpu::fpu_restore;

uint32_t cpu::fpu_size = 512;

void cpu::init() {
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    if (edx & bit_SSE) {
        wrreg(cr0, (rdreg(cr0) & ~0b100UL) | 2);
        wrreg(cr4, rdreg(cr4) | (0b11 << 9));
    }

    if (ecx & bit_XSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 18));
        uint32_t a, c;
        __cpuid_count(0xD, 0, a, eax, c, eax); // eax is no longer being used
        fpu_size = c;
        if (a & bit_XSAVEOPT)
            fpu_save = [](void *dest) { asm volatile ("xsaveopt (%0)" :: "r" (dest)); };
        else
            fpu_save = [](void *dest) { asm volatile ("xsave (%0)" :: "r" (dest)); };
        fpu_restore = [](void *dest) { asm volatile ("xrstor (%0)" :: "r" (dest)); };
    } else if (edx & bit_FXSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 9));
        fpu_save = [](void *dest) { asm volatile ("fxsave (%0)" :: "r" (dest)); };
        fpu_restore = [](void *dest) { asm volatile ("fxrstor (%0)" :: "r" (dest)); };
    }
    
    // if (ecx & bit_AVX && _fpu_save >= XSAVE)
    //     asm volatile (
    //         "mov $0, %%rcx;"
    //         "xgetbv;"
    //         "or $7, %%rax;"
    //         "xsetbv;"
    //         ::: "rdx", "rcx", "rax"
    //     );

    // __cpuid_count(7, 0, eax, ebx, ecx, edx);
    // if (ebx & (1 << 20))
    //     wrreg(cr4, rdreg(cr4) | (1 << 21));
    // if (ebx & (1 << 7))
    //     wrreg(cr4, rdreg(cr4) | (1 << 20));
    // if (ecx & (1 << 2))
    //     wrreg(cr4, rdreg(cr4) | (1 << 11));
}
