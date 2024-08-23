#include <drivers/madt.hpp>
using namespace acpi;
#define ITER_TYPE(type, enum_) for (auto iter = Iterator<type>{table, enum_}; iter; ++iter)

class madt *acpi::madt;

madt::madt(const acpi_sdt_hdr *tbl) :
    sdt{tbl},
    table{reinterpret_cast<const acpi_madt*>(tbl)},
    source_ovrds{heap::allocator()},
    ioapics(heap::allocator())
{
    lapic_addr = table->local_interrupt_controller_address;
    ITER_TYPE(acpi_madt_interrupt_source_override, ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE)
        source_ovrds.push_back(&*iter);

    ITER_TYPE(acpi_madt_ioapic, ACPI_MADT_ENTRY_TYPE_IOAPIC)
        ioapics.push_back(&*iter);
}