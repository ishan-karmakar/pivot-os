#include <cpu/idt.hpp>
#include <util/logger.h>
using namespace cpu;

void InterruptDescriptorTable::set_entry(uint8_t idx, struct idt::idt_desc desc) {
    this->idt[idx] = desc;
}

void InterruptDescriptorTable::set_entry(uint8_t idx, uint8_t ring, uintptr_t handler) {
    set_entry(idx, {
        (uint16_t) handler,
    });
}

void InterruptDescriptorTable::load_exceptions() {
}

void InterruptDescriptorTable::load() {
    idtr.addr = reinterpret_cast<uintptr_t>(&idt);
    idtr.size = idt::IDT_ENTRIES * sizeof(struct idt::idt_desc) - 1;

    asm volatile ("lidt %0" : : "rm" (idtr));
    log(Info, "IDT", "Initialized + loaded IDT");
}
