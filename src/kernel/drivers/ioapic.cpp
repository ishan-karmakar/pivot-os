#include <drivers/ioapic.hpp>
#include <util/logger.h>
#include <common.h>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
using namespace drivers;

uintptr_t IOAPIC::addr;

void IOAPIC::init(mem::PTMapper& mapper) {
    auto madt = acpi::ACPI::get_table<acpi::MADT>().value();
    auto ioapic = *madt.iter<acpi::MADT::ioapic>();
    addr = ioapic.addr;
    mapper.map(addr, addr, KERNEL_PT_ENTRY);
    mem::PMM::set(addr);
    uint32_t ioapic_version = read_reg(VERSION_OFF);
    log(Verbose, "IOAPIC", "Address: %p, GSI Base: %u, Max Redirections: %hu", ioapic.addr, ioapic.gsi_base, ioapic_version >> 16);
    log(Info, "IOAPIC", "Initialized IOAPIC");
}

void IOAPIC::set_irq(uint8_t idt_ent, uint8_t irq, uint8_t dest, uint32_t flags) {
    red_ent ent;
    ent.raw = flags;
    ent.vector = idt_ent;
    ent.dest = dest;

    auto so = find_so(irq);
    if (so.has_value()) {
        auto val = so.value();
        log(Verbose, "IOAPIC", "Found SO - IRQ %hhu -> %u", irq, val.gsi_base);
        irq = val.gsi_base;
        ent.pin_polarity = val.flags & 2;
        ent.trigger_mode = val.flags & 8;
    }

    write_red(irq, ent);
    log(Verbose, "IOAPIC", "Setting IRQ %hhu to IDT entry %hhu", irq, idt_ent);
}

void IOAPIC::set_mask(uint8_t irq, bool mask) {
    auto so = find_so(irq);
    if (so.has_value())
        irq = so.value().gsi_base;
    red_ent ent = read_red(irq);
    ent.mask = mask;
    write_red(irq, ent);
}

std::optional<acpi::MADT::ioapic_so> IOAPIC::find_so(uint8_t irq) {
    auto madt = acpi::ACPI::get_table<acpi::MADT>().value();
    for (auto source_ovrds = madt.iter<acpi::MADT::ioapic_so>(); source_ovrds; ++source_ovrds)
        if (source_ovrds->irq_source == irq)
            return std::make_optional(*source_ovrds);
    return std::nullopt;
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
