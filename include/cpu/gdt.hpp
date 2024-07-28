#pragma once
#include <cstdint>
#include <cstddef>

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
            };
            uint64_t raw;
        };

        GDT(gdt_desc*);
        ~GDT() = default;

        GDT& operator=(GDT&);
        void set_entry(uint16_t, uint8_t, uint8_t);
        void set_entry(uint16_t, gdt_desc);
        gdt_desc get_entry(uint16_t) const;
        void load();

        uint16_t entries;

    private:
        gdt_desc * const gdt;
        gdtr gdtr{ 0, reinterpret_cast<uintptr_t>(gdt) };
    };
}
