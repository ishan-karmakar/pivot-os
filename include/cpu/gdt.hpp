#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>

namespace cpu {
    namespace gdt {
        struct [[gnu::packed]] gdtr {
            uint16_t size;
            uintptr_t addr;
        };

        union desc {
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

        void early_init();
        void init();
    }

    class GDT {
    public:
        GDT(gdt::desc*);
        ~GDT() = default;

        GDT& operator=(GDT&);
        void set_entry(uint16_t, uint8_t, uint8_t);
        void set_entry(uint16_t, gdt::desc);
        gdt::desc get_entry(uint16_t) const;
        void load();

        uint16_t entries;

    private:
        gdt::desc *gdt;
        gdt::gdtr gdtr{ 0, reinterpret_cast<uintptr_t>(gdt) };
    };

    extern frg::manual_box<GDT> kgdt;
}
