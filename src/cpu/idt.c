#include <stdint.h>
#pragma pack(1)
struct idt_desc_t {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct idtr_t {
    uint16_t size;
    uint64_t address;
};
#pragma pack()

static const char *exception_names[] = {
    "Divide by Zero Error",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

__attribute__((aligned(0x10)))
struct idt_desc_t idt[256];
struct idtr_t idtr;

extern uint64_t isr_stub_table[];

void set_idt_entry(uint16_t idx, uint8_t flags, uint8_t ist, uint64_t address) {
    idt[idx].offset_low = address & 0xFFFF;
    idt[idx].offset_mid = (address >> 16) & 0xFFFF;
    idt[idx].offset_high = (address >> 32) & 0xFFFFFFFF;
    idt[idx].ist = 0;
    idt[idx].zero = 0;
    idt[idx].selector = 0x8;
    idt[idx].flags = flags;
}

void init_idt(void) {
    idtr.size = sizeof(idt) - 1;
    idtr.address = (uintptr_t) &idt;

    for (uint8_t vector = 0; vector < 32; vector++)
        set_idt_entry(vector, 0x8E, 0, isr_stub_table[vector]);

    asm volatile (
        "lidt %0\n"
        "sti"
        : : "m" (idtr)
    );
}