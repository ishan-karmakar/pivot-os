#include <cpu/idt.hpp>
#include <util/logger.h>
using namespace cpu;

void InterruptDescriptorTable::set_entry(uint8_t idx, struct idt::idt_desc desc) {
    this->idt[idx] = desc;
}

void InterruptDescriptorTable::set_entry(uint8_t idx, uint8_t ring, uintptr_t handler) {
    set_entry(idx, {
        static_cast<uint16_t>(handler),
        0x8,
        0,
        static_cast<uint8_t>(0x8E | (ring << 5)),
        static_cast<uint16_t>(handler >> 16),
        static_cast<uint32_t>(handler >> 32),
        0
    });
}

void InterruptDescriptorTable::load() {
    asm volatile ("lidt %0" : : "rm" (idtr));
    log(Info, "IDT", "Loaded IDT");
}
