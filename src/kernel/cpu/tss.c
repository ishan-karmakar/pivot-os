#include <cpu/tss.h>
#include <cpu/gdt.h>
#include <kernel/logging.h>
#include <mem/heap.h>

extern uint64_t stack[];

void init_tss(void) {
    uintptr_t kernel_tss_addr = (uintptr_t) malloc(sizeof(tss_t));
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

    log(Info, "TSS", "Initialized task state segment");
}

void set_rsp0(uintptr_t addr) {
    uint16_t tr;
    asm volatile ("str %0" : "=rm" (tr));
    gdt_desc_t *entry0 = &gdt[tr / 8];
    gdt_desc_t *entry1 = &gdt[tr / 8 + 1];
    tss_t *tss = (tss_t*) (entry0->fields.base0 |
                          (entry0->fields.base1 << 16) |
                          (entry0->fields.base2 << 24) |
                          ((entry1->raw & 0xFFFFFFFF) << 32));
    tss->rsp0 = addr;
}