#include <cpu/gdt.h>
#include <kernel/logging.h>
extern void load_gdt(uintptr_t);

gdt_desc_t gdt[] = {
    { 0 },
    { 0, 0, 0, 0b10011011, 0b00100000, 0 }, // Kernel Code
    { 0, 0, 0, 0b10010011, 0, 0 },           // Kernel Data
    {  }
};

gdtr_t gdtr;

void init_gdt(void) {
    gdtr.size = sizeof(gdt) - 1;
    gdtr.addr = (uintptr_t) &gdt;

    load_gdt((uintptr_t) &gdtr);
    log(Info, "GDT", "Initialized GDT");
}

void load_tss(tss_t *tss) {
}