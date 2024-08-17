#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <cpu/smp.hpp>
#include <cpu/tss.hpp>

#include <drivers/acpi.hpp>
#include <drivers/madt.hpp>
#include <drivers/lapic.hpp>
#include <drivers/ioapic.hpp>

#include <frg/slab.hpp>
using namespace cpu;

std::size_t smp::num_cpus = 0;

void smp::init_bsp() {
    // auto madt = drivers::acpi::get_table<drivers::MADT>();
    // for (auto iter = madt.iter<acpi_madt_lapic>(ACPI_MADT_ENTRY_TYPE_LAPIC); iter; ++iter, ++smp::num_cpus);
    // log(INFO, "SMP", "Found %lu cpus", smp::num_cpus);

    // cpu::GDT gdt{new cpu::GDT::gdt_desc[5 + num_cpus * 2]};
    // gdt.set_entry(1, 0b10011011, 0b10); // Kernel CS
    // gdt.set_entry(2, 0b10010011, 0); // Kernel DS
    // gdt.set_entry(3, 0b11111011, 0b10); // User CS
    // gdt.set_entry(4, 0b11110011, 0); // User DS

    // cpu::TSS tss{gdt};

    // cpu::IDT idt;
    // cpu::load_exceptions(idt);
    // idt.load();

    // drivers::LAPIC::init(idt);
    // drivers::IOAPIC::init();
}