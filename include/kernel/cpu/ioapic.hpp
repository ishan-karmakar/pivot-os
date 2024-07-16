#pragma once
#include <acpi/madt.hpp>
#include <mem/mapper.hpp>

namespace cpu {
    class IOAPIC {
    public:
        IOAPIC(mem::PTMapper&, mem::PMM&, acpi::MADT);
    };
}