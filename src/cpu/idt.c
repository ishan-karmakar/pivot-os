#include <stdint.h>
#include <kernel/logging.h>
#include <cpu/idt.h>

idtr_t idtr;
idt_desc_t idt[256];

void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)()) {
    idt[idx].flags = flags;
    idt[idx].ist = ist;
    idt[idx].segment_selector = selector;
    idt[idx].offset_low = (uint16_t) ((uint64_t)handler&0xFFFF);
    idt[idx].offset_mid = (uint16_t) ((uint64_t)handler >> 16);
    idt[idx].offset_high = (uint32_t)((uint64_t)handler>> 32);
    idt[idx].zero = 0;
}

void init_exceptions() {
    IDT_SET_ENTRY(0, isr0);
    IDT_SET_ENTRY(1, isr1);
    IDT_SET_ENTRY(2, isr2);
    IDT_SET_ENTRY(3, isr3);
    IDT_SET_ENTRY(4, isr4);
    IDT_SET_ENTRY(5, isr5);
    IDT_SET_ENTRY(6, isr6);
    IDT_SET_ENTRY(7, isr7);
    IDT_SET_ENTRY(8, isr8);
    IDT_SET_ENTRY(9, isr9);
    IDT_SET_ENTRY(10, isr10);
    IDT_SET_ENTRY(11, isr11);
    IDT_SET_ENTRY(12, isr12);
    IDT_SET_ENTRY(13, isr13);
    IDT_SET_ENTRY(14, isr14);
    IDT_SET_ENTRY(15, isr15);
    IDT_SET_ENTRY(16, isr16);
    IDT_SET_ENTRY(17, isr17);
    IDT_SET_ENTRY(18, isr18);
    IDT_SET_ENTRY(19, isr19);
    IDT_SET_ENTRY(20, isr20);
    IDT_SET_ENTRY(21, isr21);
    IDT_SET_ENTRY(22, isr22);
    IDT_SET_ENTRY(23, isr23);
    IDT_SET_ENTRY(24, isr24);
    IDT_SET_ENTRY(25, isr25);
    IDT_SET_ENTRY(26, isr26);
    IDT_SET_ENTRY(27, isr27);
    IDT_SET_ENTRY(28, isr28);
    IDT_SET_ENTRY(29, isr29);
    IDT_SET_ENTRY(30, isr30);
    IDT_SET_ENTRY(31, isr31);
    IDT_SET_ENTRY(255, irq255);
}

void init_idt() {
    for (int i = 0; i < 256; i++)
        idt[i] = (idt_desc_t) {0};
    init_exceptions();
    idtr.addr = (uint64_t)(uintptr_t) &idt;
    idtr.size = 256 * sizeof(idt_desc_t) - 1;
    asm volatile ("lidt %0" : : "m" (idtr));
}
