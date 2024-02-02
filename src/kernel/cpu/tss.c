#include <cpu/tss.h>
#include <cpu/gdt.h>
#include <kernel/logging.h>

extern uint64_t stack[];

tss_t tss = { 0 };

void init_tss(void) {
    tss.rsp0 = (uintptr_t) stack + 16384;

    uintptr_t tss_addr = (uintptr_t) &tss;
    uint64_t gdt_entry = (uint16_t) sizeof(tss) |
                         (tss_addr & 0xFFFF) << 16 |
                         ((tss_addr >> 16) & 0xFF) << 32 |
                         (uint64_t) 0b10001001 << 40 |
                         ((tss_addr >> 24) & 0xFF) << 56;
    set_gdt_desc(3, gdt_entry);
    gdt_entry = ((tss_addr >> 32) & 0xFFFFFFFF);
    set_gdt_desc(4, gdt_entry);

    log(Info, "TSS", "Initialized TSS");
}