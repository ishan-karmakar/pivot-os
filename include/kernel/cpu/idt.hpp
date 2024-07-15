#pragma once
#include <cstdint>
#include <cstddef>
#include <util/logger.h>

namespace cpu {
    class IDT {
    private:
        struct [[gnu::packed]] idt_desc {
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
    public:
        void set_entry(uint8_t, struct idt_desc);
        void set_entry(uint8_t, uint8_t ring, uintptr_t handler);
        void load();

    private:
        struct idt_desc idt[256];
        struct idtr idtr{
            256 * sizeof(idt_desc) - 1,
            reinterpret_cast<uintptr_t>(&idt)
        };
    };

    void load_exceptions(IDT&);
}
