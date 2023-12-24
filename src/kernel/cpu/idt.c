#include <cpu/idt.h>
#include <libc/string.h>
#include <kernel/logging.h>

idtr_t idtr;
idt_desc_t idt[256];

void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)(void)) {
    idt[idx].flags = flags;
    idt[idx].segment_selector = selector;
    idt[idx].ist = ist;
    idt[idx].offset0 = (uint16_t)(uintptr_t) handler;
    idt[idx].offset1 = (uint16_t)((uintptr_t) handler >> 16);
    idt[idx].offset2 = (uint32_t)((uintptr_t) handler >> 32);
    idt[idx].rsv = 0;
}

void load_exceptions(void) {
    IDT_SET_INT(0, isr0);
    IDT_SET_INT(1, isr1);
    IDT_SET_INT(2, isr2);
    IDT_SET_INT(3, isr3);
    IDT_SET_INT(4, isr4);
    IDT_SET_INT(5, isr5);
    IDT_SET_INT(6, isr6);
    IDT_SET_INT(7, isr7);
    IDT_SET_INT(8, isr8);
    IDT_SET_INT(9, isr9);
    IDT_SET_INT(10, isr10);
    IDT_SET_INT(11, isr11);
    IDT_SET_INT(12, isr12);
    IDT_SET_INT(13, isr13);
    IDT_SET_INT(14, isr14);
    IDT_SET_INT(15, isr15);
    IDT_SET_INT(16, isr16);
    IDT_SET_INT(17, isr17);
    IDT_SET_INT(18, isr18);
    IDT_SET_INT(19, isr19);
    IDT_SET_INT(20, isr20);
    IDT_SET_INT(21, isr21);
    IDT_SET_INT(22, isr22);
    IDT_SET_INT(23, isr23);
    IDT_SET_INT(24, isr24);
    IDT_SET_INT(25, isr25);
    IDT_SET_INT(26, isr26);
    IDT_SET_INT(27, isr27);
    IDT_SET_INT(28, isr28);
    IDT_SET_INT(29, isr29);
    IDT_SET_INT(30, isr30);
    IDT_SET_INT(31, isr31);
    // IDT_SET_INT(255, irq255);
}

void init_idt(void) {
    memset(idt, 0, sizeof(idt_desc_t) * 256);

    load_exceptions();
    idtr.size = 256 * sizeof(idt_desc_t) - 1;
    idtr.addr = (uintptr_t) &idt;

    asm volatile ("lidt %0" : : "m" (idtr));
    log(Info, "IDT", "Initialized IDT");
}
