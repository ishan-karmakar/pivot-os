#pragma once
#define IDT_SET_ENTRY(num, handler) \
    extern void handler(); \
    set_idt_entry((num), 0x8E, 0x8, 0, handler);

void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)());
void init_idt(void);