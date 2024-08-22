#include <lib/interrupts.hpp>
#include <drivers/pic.hpp>
#include <drivers/ioapic.hpp>
#include <drivers/lapic.hpp>
#include <mem/heap.hpp>
#include <lib/hash_map.hpp>
using namespace intr;

struct int_config {
    uint8_t vec;
    uint8_t dest;
    uint32_t flags;
    bool masked;
};

lib::hash_map<uint8_t, int_config, frg::hash<unsigned int>> ints;

void intr::set(uint8_t vector, uint8_t irq, uint8_t dest, uint32_t flags) {
    if (ioapic::initialized)
        ioapic::set(vector, irq, dest, flags);

    // PIC requires no additional setup, only unmasking IRQ
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