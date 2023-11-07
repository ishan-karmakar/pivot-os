#pragma once
#define IDT_SET_ENTRY(num, handler) \
    extern void handler(); \
    set_idt_entry((num), 0x8E, 0x8, 0, handler);

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

void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)());
void init_idt(void);