#include <stdint.h>
#include <kernel/logging.h>
#include <cpu/idt.h>

#pragma pack(1)
typedef struct {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_desc_t;

typedef struct {
    uint16_t size;
    uint64_t addr;
} idtr_t;
#pragma pack()

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq33();
extern void irq34();
extern void irq255();

idtr_t idtr;
idt_desc_t idt[256];

void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)() ){
    idt[idx].flags = flags;
    idt[idx].ist = ist;
    idt[idx].segment_selector = selector;
    idt[idx].offset_low = (uint16_t) ((uint64_t)handler&0xFFFF);
    idt[idx].offset_mid = (uint16_t) ((uint64_t)handler >> 16);
    idt[idx].offset_high = (uint32_t)((uint64_t)handler>> 32);
    idt[idx].zero = 0;
}

void init_exceptions() {
    set_idt_entry(0, 0x8E, 0x8, 0, isr0);
    set_idt_entry(1, 0x8E, 0x8, 0, isr1);
    set_idt_entry(2, 0x8E, 0x8, 0, isr2);
    set_idt_entry(3, 0x8E, 0x8, 0, isr3);
    set_idt_entry(4, 0x8E, 0x8, 0, isr4);
    set_idt_entry(5, 0x8E, 0x8, 0, isr5);
    set_idt_entry(6, 0x8E, 0x8, 0, isr6);
    set_idt_entry(7, 0x8E, 0x8, 0, isr7);
    set_idt_entry(8, 0x8E, 0x8, 0, isr8);
    set_idt_entry(9, 0x8E, 0x8, 0, isr9);
    set_idt_entry(10, 0x8E, 0x8, 0, isr10);
    set_idt_entry(11, 0x8E, 0x8, 0, isr11);
    set_idt_entry(12, 0x8E, 0x8, 0, isr12);
    set_idt_entry(13, 0x8E, 0x8, 0, isr13);
    set_idt_entry(14, 0x8E, 0x8, 0, isr14);
    set_idt_entry(15, 0x8E, 0x8, 0, isr15);
    set_idt_entry(16, 0x8E, 0x8, 0, isr16);
    set_idt_entry(17, 0x8E, 0x8, 0, isr17);
    set_idt_entry(18, 0x8E, 0x8, 0, isr18);
    set_idt_entry(19, 0x8E, 0x8, 0, isr19);
    set_idt_entry(20, 0x8E, 0x8, 0, isr20);
    set_idt_entry(21, 0x8E, 0x8, 0, isr21);
    set_idt_entry(22, 0x8E, 0x8, 0, isr22);
    set_idt_entry(23, 0x8E, 0x8, 0, isr23);
    set_idt_entry(24, 0x8E, 0x8, 0, isr24);
    set_idt_entry(25, 0x8E, 0x8, 0, isr25);
    set_idt_entry(26, 0x8E, 0x8, 0, isr26);
    set_idt_entry(27, 0x8E, 0x8, 0, isr27);
    set_idt_entry(28, 0x8E, 0x8, 0, isr28);
    set_idt_entry(29, 0x8E, 0x8, 0, isr29);
    set_idt_entry(30, 0x8E, 0x8, 0, isr30);
    set_idt_entry(31, 0x8E, 0x8, 0, isr31);
    set_idt_entry(33, 0x8E, 0x8, 0, irq33);
    set_idt_entry(34, 0x8E, 0x8, 0, irq34);
    set_idt_entry(255, 0x8E, 0x8, 0, irq255);
}

void init_idt(){
    for (int i = 0; i < 256; i++)
        idt[i] = (idt_desc_t) {0};
    init_exceptions();
    idtr.addr = (uint64_t)(uintptr_t) &idt;
    idtr.size = 256 * sizeof(idt_desc_t) - 1;
    asm volatile ("lidt %0" : : "m" (idtr));
}
