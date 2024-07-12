#include <cpu/idt.hpp>
#include <util/logger.h>
#define SET_ENTRY(idx, handler) \
    extern void handler(); \
    set_entry(idx, 0, (uintptr_t) handler);

using namespace cpu;

void InterruptDescriptorTable::set_entry(uint8_t idx, struct idt::idt_desc desc) {
    this->idt[idx] = desc;
}

void InterruptDescriptorTable::set_entry(uint8_t idx, uint8_t ring, uintptr_t handler) {
    set_entry(idx, {
        static_cast<uint16_t>(handler)
    });
}

void InterruptDescriptorTable::load_exceptions() {
    SET_ENTRY(0, isr0);
    SET_ENTRY(1, isr1);
    SET_ENTRY(2, isr2);
    SET_ENTRY(3, isr3);
    SET_ENTRY(4, isr4);
    SET_ENTRY(5, isr5);
    SET_ENTRY(6, isr6);
    SET_ENTRY(7, isr7);
    SET_ENTRY(8, isr8);
    SET_ENTRY(9, isr9);
    SET_ENTRY(10, isr10);
    SET_ENTRY(11, isr11);
    SET_ENTRY(12, isr12);
    SET_ENTRY(13, isr13);
    SET_ENTRY(14, isr14);
    SET_ENTRY(15, isr15);
    SET_ENTRY(16, isr16);
    SET_ENTRY(17, isr17);
    SET_ENTRY(18, isr18);
    SET_ENTRY(19, isr19);
    SET_ENTRY(20, isr20);
    SET_ENTRY(21, isr21);
    SET_ENTRY(22, isr22);
    SET_ENTRY(23, isr23);
    SET_ENTRY(24, isr24);
    SET_ENTRY(25, isr25);
    SET_ENTRY(26, isr26);
    SET_ENTRY(27, isr27);
    SET_ENTRY(28, isr28);
    SET_ENTRY(29, isr29);
    SET_ENTRY(30, isr30);
    SET_ENTRY(31, isr31);
}

void InterruptDescriptorTable::load() {
    load_exceptions();
    idtr.addr = reinterpret_cast<uintptr_t>(&idt);
    idtr.size = idt::IDT_ENTRIES * sizeof(struct idt::idt_desc) - 1;

    asm volatile ("lidt %0" : : "rm" (idtr));
    log(Info, "IDT", "Initialized + loaded IDT");
}
