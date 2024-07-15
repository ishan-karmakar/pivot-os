#pragma once
#include <cstdint>
#include <cstddef>
#include <util/logger.h>

namespace cpu {
    template<uint16_t L>
    class GDT {
    private:
        struct [[gnu::packed]] gdtr {
            uint16_t size;
            uintptr_t addr;
        };

        struct [[gnu::packed]] alignas(8) gdt_desc {
            uint16_t limit0;
            uint16_t base0;
            uint8_t base1;
            uint8_t access_byte;
            uint8_t limit1:4;
            uint8_t flags:4;
            uint8_t base2;
        };

    public:
        void set_entry(uint16_t idx, struct gdt_desc desc) {
            if (idx >= L)
                log(Warning, "GDT", "Index more than max GDT size");
            gdt[idx] = desc;
        }

        void set_entry(uint16_t idx, uint8_t access, uint8_t flags) {
            struct gdt_desc desc { 0xFFFF, 0, 0, access, 0xF, flags, 0 };

            set_entry(idx, desc);
        }

        void load() {
            asm volatile (
                "cli;"
                "lgdt %0;"
                "push $0x8;"
                "push $.L%=;"
                "lretq;"
                ".L%=:"
                "mov %1, %%ds;"
                "mov %1, %%es;"
                "mov %1, %%fs;"
                "mov %1, %%gs;"
                "mov %1, %%ss;"
                : : "rm" (gdtr), "r" (0x10) : "memory"
            );
            log(Info, "GDT", "Loaded GDT");
        };

    private:
        struct gdt_desc gdt[L];
        struct gdtr gdtr{
            L * sizeof(struct gdt_desc) - 1,
            reinterpret_cast<uintptr_t>(&gdt)
        };
    };
}
