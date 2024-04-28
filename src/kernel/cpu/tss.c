#include <cpu/tss.h>
#include <cpu/gdt.h>
#include <kernel/logging.h>

extern uint64_t stack[];

tss_t kernel_tss = { 0 };

void init_tss(void) {
    // kernel_tss.rsp0 = (uintptr_t) stack + 16384;
    uintptr_t kernel_tss_addr = (uintptr_t) &kernel_tss;
    uint64_t gdt_entry = (uint16_t) sizeof(kernel_tss) |
                         (kernel_tss_addr & 0xFFFF) << 16 |
                         ((kernel_tss_addr >> 16) & 0xFF) << 32 |
                         (uint64_t) 0b10001001 << 40 |
                         ((kernel_tss_addr >> 24) & 0xFF) << 56;
    set_gdt_desc(5, gdt_entry);
    gdt_entry = ((kernel_tss_addr >> 32) & 0xFFFFFFFF);
    set_gdt_desc(6, gdt_entry);

    log(Info, "TSS", "Initialized TSS");
}

void set_rsp0(uintptr_t addr) { kernel_tss.rsp0 = addr; }