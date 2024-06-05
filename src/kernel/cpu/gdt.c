#include <cpu/gdt.h>
#include <kernel/logging.h>
extern void load_gdt(uintptr_t);
uint16_t gdt_entries = 5;

gdt_desc_t gdt[MAX_GDT_ENTRIES] = {
    { { 0 } },
    { { 0xFFFF, 0, 0, 0b10011011, 0xF | 0b00100000, 0 } }, // Kernel Code
    { { 0xFFFF, 0, 0, 0b10010011, 0xF, 0 } },           // Kernel Data
    { { 0xFFFF, 0, 0, 0b11111011, 0xF | 0b00100000, 0 } }, // User Code
    { { 0xFFFF, 0, 0, 0b11110011, 0xF, 0 } }, // User Data
};

gdtr_t gdtr = { 0, (uintptr_t) &gdt };

void init_gdt(void) {
    gdtr.size = gdt_entries * sizeof(gdt_desc_t) - 1;

    load_gdt((uintptr_t) &gdtr);
    log(Info, "GDT", "Initialized GDT");
}

void set_gdt_desc(uint16_t idx, uint64_t entry) {
    gdt[idx].raw = entry;
    if (idx >= gdt_entries) {
        gdt_entries = idx + 1;
        gdtr.size = gdt_entries * sizeof(gdt_desc_t) - 1;
    }
}
