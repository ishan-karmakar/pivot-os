#pragma once
#include <cstdint>
#include <cstddef>
#include <util/logger.h>
#include <algorithm>

namespace cpu {
    class GDT {
    private:
        struct [[gnu::packed]] gdtr {
            uint16_t size;
            uintptr_t addr;
        };
    
    public:
        union gdt_desc {
            struct [[gnu::packed]] alignas(8) {
                uint16_t limit0;
                uint16_t base0;
                uint8_t base1;
                uint8_t access_byte;
                uint8_t limit1:4;
                uint8_t flags:4;
                uint8_t base2;
            } field;
            uint64_t raw;
        };

        GDT(gdt_desc *gdt) : entries{1}, gdt{gdt} { gdt[0] = {}; }
        ~GDT() = default;

        GDT& operator=(GDT& old) {
            for (uint16_t i = 0; i < old.entries; i++)
                set_entry(i, old.get_entry(i));
            entries = std::max(entries, old.entries);
            return *this;
        }

        void set_entry(uint16_t idx, uint8_t access, uint8_t flags) {
            gdt_desc desc { .field = { 0xFFFF, 0, 0, access, 0xF, flags, 0 } };

            set_entry(idx, desc);
        }

        void set_entry(uint16_t idx, gdt_desc desc) {
            gdt[idx] = desc;
            if (idx >= entries)
                entries = idx + 1;
        }

        gdt_desc get_entry(uint16_t idx) { return gdt[idx]; }

        void load() {
            gdtr.size = entries * sizeof(gdt_desc) - 1;
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

        uint16_t entries;

    private:
        gdt_desc *gdt;
        gdtr gdtr{ 0, reinterpret_cast<uintptr_t>(gdt) };
    };
}
