#include <cpu/gdt.h>
#include <kernel/logging.h>
extern void load_gdt(uintptr_t);

gdt_desc_t gdt[] = {
    { { 0 } },
    { { 0, 0, 0, 0b10011011, 0b00100000, 0 } }, // Kernel Code
    { { 0, 0, 0, 0b10010011, 0, 0 } },           // Kernel Data
    { { 0, 0, 0, 0b11111011, 0b00100000, 0 } }, // User Code
    { { 0, 0, 0, 0b11110011, 0, 0 } }, // User Data
    { { 0 } }, // TSS Low
    { { 0 } }  // TSS High
};

gdtr_t gdtr;

void init_gdt(void) {
    gdtr.size = sizeof(gdt) - 1;
    gdtr.addr = (uintptr_t) &gdt;
    log(Info, "GDT", "Initialized GDT");

    load_gdt((uintptr_t) &gdtr);
    log(Info, "GDT", "Loaded GDT and TSS");
}

void set_gdt_desc(uint16_t idx, uint64_t entry) {
    gdt[idx].raw = entry;
}
