#include <cpu/ioapic.hpp>
#include <util/logger.h>

using namespace cpu;

IOAPIC::IOAPIC(mem::PTMapper& mapper, mem::PMM& pmm, acpi::MADT madt) {
    auto ioapic = *madt.iter<acpi::MADT::ioapic>();
    log(Verbose, "IOAPIC", "Address: %x, GSI Base: %u", ioapic.addr, ioapic.gsi_base);
}
