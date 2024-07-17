#pragma once
#include <mem/mapper.hpp>
#include <acpi/madt.hpp>
#include <cpu/idt.hpp>
#include <cpu/ioapic.hpp>

#define APIC_SPURIOUS_IDT_ENT 0xFF
#define APIC_PERIODIC_IDT_ENT 32

namespace cpu {
    class LAPIC {
    public:
        LAPIC() = delete;

        static void init(mem::PTMapper&, IDT&);
        static void init();
        static void calibrate(IOAPIC&);
        static inline void eoi() { write_reg(0xB0, 0); }

        static uint32_t ms_interval;

    private:
        enum timer_div {
            Div1 = 0xB,
            Div2 = 0,
            Div4 = 1,
            Div8 = 2,
            Div16 = 3,
            Div32 = 8,
            Div64 = 9,
            Div128 = 0xA
        };

        static void write_reg(uint32_t, uint64_t);
        static uint64_t read_reg(uint32_t);

        static bool x2mode;
        static uintptr_t lapic;
    };
}
