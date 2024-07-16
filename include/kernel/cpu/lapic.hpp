#pragma once
#include <mem/mapper.hpp>
#include <acpi/madt.hpp>
#include <cpu/idt.hpp>

#define APIC_SPURIOUS_IDT_ENT 0xFF
#define APIC_PERIODIC_IDT_ENT 33

namespace cpu {
    class LAPIC {
    public:
        LAPIC(mem::PTMapper&, mem::PMM&, IDT&, const acpi::MADT);

    private:
        void write_reg(uint32_t, uint64_t) const;
        uint64_t read_reg(uint32_t) const;

        bool x2mode;
        uintptr_t lapic;
    };
}
