#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>

namespace cpu {
    namespace idt {
        struct [[gnu::packed]] desc {
            uint16_t offset0;
            uint16_t segment_selector;
            uint8_t ist;
            uint8_t flags;
            uint16_t offset1;
            uint32_t offset2;
            uint32_t rsv;
        };

        struct [[gnu::packed]] idtr {
            uint16_t size;
            uintptr_t addr;
        };

        void load_exceptions();
        void init();
    }

    class IDT {
    public:
        void set_entry(uint8_t, struct idt::desc);
        void set_entry(uint8_t, uint8_t ring, void (*handler)());
        void load() const;

    private:
        struct idt::desc idt[256];
        struct idt::idtr idtr{
            256 * sizeof(idt::desc) - 1,
            reinterpret_cast<uintptr_t>(&idt)
        };
    };

    extern frg::manual_box<IDT> kidt;
}
