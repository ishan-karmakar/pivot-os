#include <drivers/interrupts.hpp>
#include <drivers/pic.hpp>
#include <drivers/ioapic.hpp>
#include <drivers/lapic.hpp>
using namespace interrupts;

void interrupts::set(uint8_t vector, uint8_t irq, std::pair<uint8_t, uint32_t> config) {
    if (ioapic::initialized)
        ioapic::set(vector, irq, config);

    // PIT requires no additional setup, only unmasking IRQ
}

void interrupts::mask(uint8_t irq) {
    if (ioapic::initialized)
        ioapic::mask(irq);
    else
        pic::mask(irq);
}

void interrupts::unmask(uint8_t irq) {
    if (ioapic::initialized)
        ioapic::unmask(irq);
    else
        pic::unmask(irq);
}

void interrupts::eoi(uint8_t irq) {
    if (ioapic::initialized)
        lapic::eoi();
    else
        pic::eoi(irq);
}