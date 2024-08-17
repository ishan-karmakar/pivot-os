#include <drivers/ioapic.hpp>
#include <lib/logger.hpp>
#include <kernel.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <drivers/acpi.hpp>
#include <drivers/madt.hpp>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
using namespace drivers;

uintptr_t IOAPIC::addr = 0;
bool IOAPIC::initialized = false;

void IOAPIC::init() {
    if (initialized) return;
    // auto madt = acpi::get_table<drivers::MADT>();
    // auto mioapic = *madt.iter<acpi_madt_ioapic>(ACPI_MADT_ENTRY_TYPE_IOAPIC);
    // addr = mioapic.address;
    // mem::PMM::set(addr);
    // uint32_t ioapic_version = read_reg(VERSION_OFF);
    // log(VERBOSE, "IOAPIC", "Address: %p, GSI Base: %u, Max Redirections: %hu", addr, mioapic.gsi_base, ioapic_version >> 16);
    // log(INFO, "IOAPIC", "Initialized IOAPIC");
    // initialized = true;
    // if (uacpi_unlikely(uacpi_namespace_load())) {
    //     logger::error("IOAPIC", "uACPI failed to load namespaces");
    //     abort();
    // }
    // if (uacpi_unlikely(uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC))) {
    //     logger::error("IOAPIC", "uACPI failed to set interrupt model");
    //     abort();
    // }
}

void IOAPIC::set_irq(uint8_t idt_ent, uint8_t irq, uint8_t dest, uint32_t flags) {
    red_ent ent;
    ent.raw = flags;
    ent.vector = idt_ent;
    ent.dest = dest;

    auto so = find_so(irq);
    if (so.has_value()) {
        auto val = so.value();
        logger::verbose("IOAPIC", "Found SO - IRQ %hhu -> %u", irq, val.gsi);
        irq = val.gsi;
        ent.pin_polarity = val.flags & 2;
        ent.trigger_mode = val.flags & 8;
    }

    write_red(irq, ent);
    logger::verbose("IOAPIC", "Setting IRQ %hhu to IDT entry %hhu", irq, idt_ent);
}

void IOAPIC::set_mask(uint8_t irq, bool mask) {
    auto so = find_so(irq);
    if (so.has_value())
        irq = so.value().gsi;
    red_ent ent = read_red(irq);
    ent.mask = mask;
    write_red(irq, ent);
}

frg::optional<acpi_madt_interrupt_source_override> IOAPIC::find_so(uint8_t) {
    // auto madt = acpi::get_table<drivers::MADT>();
    // for (auto source_ovrds = madt.iter<acpi_madt_interrupt_source_override>(ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE); source_ovrds; ++source_ovrds)
    //     if (source_ovrds->source == irq)
    //         return frg::optional(*source_ovrds);
    return frg::null_opt;
}

uint32_t IOAPIC::read_reg(uint32_t off) {
    *(volatile uint32_t*) addr = off;
    return *(volatile uint32_t*) (addr + 0x10);
}

void IOAPIC::write_reg(uint32_t off, uint32_t val) {
    *(volatile uint32_t*) addr = off;
    *(volatile uint32_t*) (addr + 0x10) = val;
}

IOAPIC::red_ent IOAPIC::read_red(uint8_t irq) {
    irq *= 2;
    irq += 0x10;
    uint32_t lower = read_reg(irq);
    uint64_t upper = read_reg(irq + 1);
    return { .raw = (upper << 32) | lower };
}

void IOAPIC::write_red(uint8_t irq, const red_ent& ent) {
    irq *= 2;
    irq += 0x10;
    uint32_t lower = ent.raw;
    uint32_t upper = ent.raw >> 32;

    write_reg(irq, lower);
    write_reg(irq + 1, upper);
}
