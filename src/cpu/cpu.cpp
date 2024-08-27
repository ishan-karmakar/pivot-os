#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <cpuid.h>
using namespace cpu;

void enable_smap();
void enable_smep();
void detect_fpu();

constexpr uint32_t IA32_EFER = 0xC0000080;

enum {
    NoneSave,
    FXSAVE,
    XSAVE,
    XSAVEOPT,
} _fpu_save;

// TODO: Use std::function for fpu_save and fpu_restore instead of enum; will be faster

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
        _fpu_save = a & bit_XSAVEOPT ? XSAVEOPT : XSAVE;
    } else if (edx & bit_FXSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 9));
        _fpu_save = FXSAVE;
    } else
        _fpu_save = NoneSave;
    
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

void cpu::fpu_save(void *dest) {
    switch (_fpu_save) {
    case FXSAVE:
        asm volatile ("fxsave (%0)" :: "r" (dest));
        break;
    case XSAVE:
        asm volatile ("xsave (%0)" :: "r" (dest));
        break;
    case XSAVEOPT:
        asm volatile ("xsaveopt (%0)" :: "r" (dest));
        break;
    case NoneSave:
        break;
    }
}

void cpu::fpu_restore(void *dest) {
    if (_fpu_save == FXSAVE)
        asm volatile ("fxrstor (%0)" :: "r" (dest));
    else if (_fpu_save >= XSAVE)
        asm volatile ("xrstor (%0)" :: "r" (dest));
}
