#include <cpu/tss.h>
#include <cpu/gdt.h>
#include <util/logger.h>
#include <mem/heap.h>
#include <kernel.h>

extern uint64_t stack[];

void init_tss(void) {
    uintptr_t kernel_tss_addr = (uintptr_t) halloc(sizeof(tss_t), KHEAP);
    uint64_t gdt_entry = (uint16_t) sizeof(tss_t) |
                         (kernel_tss_addr & 0xFFFF) << 16 |
                         ((kernel_tss_addr >> 16) & 0xFF) << 32 |
                         (uint64_t) 0b10001001 << 40 |
                         ((kernel_tss_addr >> 24) & 0xFF) << 56;
    uint16_t tss_reg = gdt_entries * 8;
    set_gdt_desc(gdt_entries, gdt_entry);
    gdt_entry = ((kernel_tss_addr >> 32) & 0xFFFFFFFF);
    set_gdt_desc(gdt_entries, gdt_entry);

    load_gdt((uintptr_t) &gdtr);
    asm volatile ("ltr %0" : : "r" (tss_reg));

    log(Info, "TSS", "Initialized TSS");
}