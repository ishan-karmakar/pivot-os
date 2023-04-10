#include <stdint.h>
#include "idt.h"
#include "log.h"
extern void* isr_stub_table[];
#define IDT_ENTRIES 32

struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t seg_sel;
    uint8_t ist; // Only first 3 bits are used
    uint8_t attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t rsv;
};

struct __attribute__((packed)) idtr {
    uint16_t size;
    uint64_t base;
};

static struct idt_entry idt[IDT_ENTRIES];
static struct idtr idtr;

static void set_entry(struct idt_entry* entry, void* isr, uint8_t flags) {
    uint64_t isr_addr = (uint64_t)(uintptr_t) isr;
    entry->offset_low = isr_addr & 0xFFFF;
    entry->seg_sel = 0x8;
    entry->ist = 0;
    entry->attributes = flags;
    entry->offset_mid = (isr_addr >> 16) & 0xFFFF;
    entry->offset_high = (isr_addr >> 32) & 0xFFFFFFFF;
    entry->rsv = 0;
}

void load_idt(void) {
    idtr.base = (uintptr_t) &idt[0];
    idtr.size = sizeof(struct idt_entry) * IDT_ENTRIES;

    for (uint8_t i = 0; i < 32; i++)
        set_entry(&idt[i], isr_stub_table[i], 0x8E);

    asm ("lidt (idtr)");
    asm ("sti");

    log(Info, "IDT", "Initialized IDT");
}
