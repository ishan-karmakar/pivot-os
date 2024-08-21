#include <lib/interrupts.hpp>
#include <drivers/pic.hpp>
#include <drivers/ioapic.hpp>
#include <drivers/lapic.hpp>
using namespace intr;

// TODO: Save mappings between vectors and irqs
// When switching from PIC to IOAPIC, need to transfer mappings somehow
// Right now plan is to just use a vector

void intr::set(uint8_t vector, uint8_t irq, std::pair<uint8_t, uint32_t> config) {
    if (ioapic::initialized)
        ioapic::set(vector, irq, config);

    // PIT requires no additional setup, only unmasking IRQ
    mask(irq);
}

void intr::mask(uint8_t irq) {
    if (ioapic::initialized)
        ioapic::mask(irq);
    else
        pic::mask(irq);
}

void intr::unmask(uint8_t irq) {
    if (ioapic::initialized)
        ioapic::unmask(irq);
    else
        pic::unmask(irq);
}

void intr::eoi(uint8_t irq) {
    if (ioapic::initialized)
        lapic::eoi();
    else
        pic::eoi(irq);
}