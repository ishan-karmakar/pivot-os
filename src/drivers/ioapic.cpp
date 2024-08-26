#include <drivers/ioapic.hpp>
#include <lib/logger.hpp>
#include <optional>
#include <uacpi/acpi.h>
#include <drivers/acpi.hpp>
#include <drivers/madt.hpp>
#include <drivers/pic.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <lib/interrupts.hpp>
using namespace ioapic;

constexpr int VERSION_OFF = 1;

union red_ent {
    struct {
        uint8_t vector;
        uint8_t delivery_mode:3;
        uint8_t dest_mode:1;
        uint8_t delivery_status:1;
        uint8_t pin_polarity:1;
        uint8_t remote_irr:1;
        uint8_t trigger_mode:1;
        uint8_t mask:1;
        uint64_t rsv:39;
        uint8_t dest;
    };
    uint64_t raw;
};

uint32_t read_reg(uint32_t);
void write_reg(uint32_t, uint32_t);
red_ent read_red(uint8_t);
void write_red(uint8_t, const red_ent&);
std::optional<acpi_madt_interrupt_source_override> find_so(uint8_t);

static uintptr_t addr;
bool ioapic::initialized = false;

void ioapic::init() {
    pic::disable();
    auto mioapic = acpi::madt->ioapics[0];
    logger::verbose("IOAPIC", "IOAPIC Address: %p, GSI Base: %u", mioapic->address, mioapic->gsi_base);
    pmm::set(mioapic->address);
    addr = virt_addr(static_cast<uintptr_t>(mioapic->address));
    initialized = true;
    intr::transfer_ints();
    logger::info("IOAPIC", "Initialized IOAPIC");

    // if (uacpi_unlikely(uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC))) {
    //     logger::error("IOAPIC", "uACPI failed to set interrupt model");
    //     abort();
    // }
}

void ioapic::set(uint8_t idt_ent, uint8_t irq, uint8_t dest, uint32_t flags) {
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

void mask(uint8_t irq, bool m) {
    auto so = find_so(irq);
    if (so.has_value())
        irq = so.value().gsi;
    red_ent ent = read_red(irq);
    ent.mask = m;
    write_red(irq, ent);
}

void ioapic::mask(uint8_t irq) { ::mask(irq, true); }
void ioapic::unmask(uint8_t irq) { ::mask(irq, false); }

std::optional<acpi_madt_interrupt_source_override> find_so(uint8_t irq) {
    for (auto source_ovrds : acpi::madt->source_ovrds)
        if (source_ovrds->source == irq)
            return std::optional(*source_ovrds);
    return std::nullopt;
}

uint32_t read_reg(uint32_t off) {
    *(volatile uint32_t*) addr = off;
    return *(volatile uint32_t*) (addr + 0x10);
}

void write_reg(uint32_t off, uint32_t val) {
    *(volatile uint32_t*) addr = off;
    *(volatile uint32_t*) (addr + 0x10) = val;
}

red_ent read_red(uint8_t irq) {
    irq *= 2;
    irq += 0x10;
    uint32_t lower = read_reg(irq);
    uint64_t upper = read_reg(irq + 1);
    return { .raw = (upper << 32) | lower };
}

void write_red(uint8_t irq, const red_ent& ent) {
    irq *= 2;
    irq += 0x10;
    uint32_t lower = ent.raw;
    uint32_t upper = ent.raw >> 32;

    write_reg(irq, lower);
    write_reg(irq + 1, upper);
}
