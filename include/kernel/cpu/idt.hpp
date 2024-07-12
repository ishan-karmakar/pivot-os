#pragma once
#include <cstdint>
#include <cstddef>

namespace cpu {
    namespace idt {
        const size_t IDT_ENTRIES = 256;

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
    }

    class InterruptDescriptorTable {
    public:
        void set_entry(uint8_t, struct idt::idt_desc);
        void set_entry(uint8_t, uint8_t ring, uintptr_t handler);
        void load();

    private:
        struct idt::idt_desc idt[idt::IDT_ENTRIES];
        struct idt::idtr idtr;
    };

    namespace isr {
        void load_exceptions(cpu::InterruptDescriptorTable&);
    }
}
