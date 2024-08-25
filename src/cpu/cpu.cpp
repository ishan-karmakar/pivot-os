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
} fpu_save;

enum {
    XRSTOR,
    FXRSTOR,
    NoneRestore
} fpu_restore;

uint32_t fpu_storage_size;

void cpu::init() {
    // Enable SSE
    uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    detect_fpu();

    if (edx & (1 << 25)) {
        wrreg(cr0, (rdreg(cr0) & ~0b100UL) | 2);
        wrreg(cr4, rdreg(cr4) | 0x600);

        if (ecx & (1 << 28) && fpu_restore == XRSTOR) {
            asm volatile (
                "mov $0, %%rcx;"
                "xgetbv;"
                "or $7, %%rax;"
                "xsetbv;"
                : : : "rdx", "rcx", "rax"
            );
        }
    }

    // TODO: Enable these back once I get basic scheduling working
    // enable_smap();
    // enable_smep();
    // TODO: UMIP
}

[[gnu::always_inline]]
inline void enable_smap() {
    uint32_t ignored, ebx;
    __cpuid_count(7, 0, ignored, ebx, ignored, ignored);
    if (ebx & (1 << 20))
        wrreg(cr4, rdreg(cr4) | (1 << 21));
}

[[gnu::always_inline]]
inline void enable_smep() {
    uint32_t ignored, ebx;
    __cpuid_count(7, 0, ignored, ebx, ignored, ignored);
    if (ebx & (1 << 7))
        wrreg(cr4, rdreg(cr4) | (1 << 20));
}

[[gnu::always_inline]]
inline void detect_fpu() {
    uint32_t a, b, c, d;
    __cpuid(1, a, b, c, d);
    if (c & bit_XSAVE) {
        // XSAVE supported
        wrreg(cr4, rdreg(cr4) | (1 << 18));
        __cpuid_count(0xD, 0, a, b, c, d);

        fpu_storage_size = c;
        fpu_restore = XRSTOR;

        __cpuid_count(0xD, 1, a, b, c, d);
        fpu_save = a & bit_XSAVEOPT ? XSAVEOPT : XSAVE;
    } else if (d & bit_FXSAVE) {
        wrreg(cr4, rdreg(cr4) | (1 << 9));

        fpu_storage_size = 512;
        fpu_save = FXSAVE;
        fpu_restore = FXRSTOR;
    } else {
        fpu_save = NoneSave;
        fpu_restore = NoneRestore;
    }
}

extern "C" {
    void fpu_sav(cpu::status *status) {
        if (fpu_save == NoneSave) return;
        status->fpu_data = operator new(fpu_storage_size);
        switch (fpu_save) {
        case FXSAVE:
            asm volatile ("fxsave (%0)" :: "r" (status->fpu_data));
            break;
        case XSAVE:
            asm volatile ("xsave (%0)" :: "r" (status->fpu_data));
            break;
        case XSAVEOPT:
            asm volatile ("xsaveopt (%0)" :: "r" (status->fpu_data));
            break;
        case NoneSave:
            break;
        }
    }

    cpu::status *fpu_rest(cpu::status *status) {
        switch (fpu_restore) {
        case FXRSTOR:
            asm volatile ("fxrstor (%0)" :: "r" (status->fpu_data));
            break;
        case XRSTOR:
        }
    }
}
