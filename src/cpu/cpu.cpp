#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <cpuid.h>
using namespace cpu;

void enable_smap();
void enable_smep();
void detect_fpu();

constexpr uint32_t IA32_EFER = 0xC0000080;

enum {
    XSAVEOPT,
    XSAVE,
    FXSAVE,
    NoneSave
} _fpu_save;

uint32_t cpu::fpu_size;

void cpu::init() {
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    if (ecx & bit_XSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 18));
        __cpuid_count(0xD, 0, eax, ebx, ecx, edx);
        _fpu_save = eax & bit_XSAVEOPT ? XSAVEOPT : XSAVE;
    } else if (edx & bit_FXSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 9));
        _fpu_save = FXSAVE;
    } else
        _fpu_save = NoneSave;

    // detect_fpu();

    // if (edx & (1 << 25)) {
    //     wrreg(cr0, (rdreg(cr0) & ~0b100UL) | 2);
    //     wrreg(cr4, rdreg(cr4) | 0x600);

    //     if (ecx & (1 << 28) && _fpu_rest == XRSTOR) {
    //         asm volatile (
    //             "mov $0, %%rcx;"
    //             "xgetbv;"
    //             "or $7, %%rax;"
    //             "xsetbv;"
    //             : : : "rdx", "rcx", "rax"
    //         );
    //     }
    // }

    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    if (ebx & (1 << 20))
        wrreg(cr4, rdreg(cr4) | (1 << 21));
    if (ebx & (1 << 7))
        wrreg(cr4, rdreg(cr4) | (1 << 20));
    if (ecx & (1 << 2))
        wrreg(cr4, rdreg(cr4) | (1 << 11));
}

[[gnu::always_inline]]
inline void detect_fpu() {
    uint32_t a, b, c, d;
    __cpuid(1, a, b, c, d);
    if (c & bit_XSAVE) {
        // XSAVE supported
        wrreg(cr4, rdreg(cr4) | (1 << 18));
        __cpuid_count(0xD, 0, a, b, c, d);

        fpu_size = c;
        _fpu_rest = XRSTOR;

        __cpuid_count(0xD, 1, a, b, c, d);
        _fpu_save = a & bit_XSAVEOPT ? XSAVEOPT : XSAVE;
    } else if (d & bit_FXSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 9));

        fpu_size = 512;
        _fpu_save = FXSAVE;
        _fpu_rest = FXRSTOR;
    } else {
        _fpu_save = NoneSave;
        _fpu_rest = NoneRestore;
    }
}

void cpu::fpu_save(void *dest) {
    if (_fpu_save == NoneSave) return;
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
    switch (_fpu_rest) {
    case FXRSTOR:
        asm volatile ("fxrstor (%0)" :: "r" (dest));
        break;
    case XRSTOR:
        asm volatile ("xrstor (%0)" :: "r" (dest));
        break;
    case NoneRestore:
        return;
    }
}
