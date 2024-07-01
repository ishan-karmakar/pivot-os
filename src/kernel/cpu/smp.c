#include <cpu/smp.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/lapic.h>
#include <cpu/cpu.h>
#include <acpi/madt.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <util/logger.h>
#include <libc/string.h>
#include <scheduler/thread.h>
#include <kernel.h>

extern void ap_trampoline(void);
extern gdtr_t gdtr;
extern idtr_t idtr;

void start_ap(uint64_t, uint8_t);

volatile ap_info_t *ap_boot = (ap_info_t*) 0x8010;
void start_aps(void) {
    map_addr(0x8000, 0x8000, KERNEL_PT_ENTRY, KMEM.pml4);
    memcpy((void*) 0x8000, &ap_trampoline, PAGE_SIZE);
    ap_boot->gdtr = (uintptr_t) &gdtr;
    ap_boot->idtr = (uintptr_t) &idtr;
    ap_boot->pml4 = PADDR(KMEM.pml4);
    log(Verbose, "SMP", "%u", CPU);
    for (size_t id = 0; id < KSMP.num_cpus; id++) {
        if (id == CPU)
            continue;
        ap_boot->stack_top = (uintptr_t) alloc_frame() + PAGE_SIZE;
        KCPUS[id].stack = ap_boot->stack_top - PAGE_SIZE;
        map_addr(KCPUS[id].stack, VADDR(KCPUS[id].stack), KERNEL_PT_ENTRY, KSMP.idle->vmm.p4_tbl);
        map_addr(ap_boot->stack_top - PAGE_SIZE, ap_boot->stack_top - PAGE_SIZE, KERNEL_PT_ENTRY, KMEM.pml4);
        start_ap(id, 0x8);
        while (!ap_boot->ready) asm ("pause");
        pmm_clear_bit(ap_boot->stack_top - PAGE_SIZE);
    }

    pmm_clear_bit(0x8000);
    // log(Info, "SMP", "All CPUs booted up");
}

void start_ap(uint64_t id, uint8_t trampoline_page) {
    // write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, (id << 32) | ICR_INIT | ICR_ASSERT | ICR_LEVEL);
    // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    // write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, (id << 32) | ICR_INIT | ICR_LEVEL);
    // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    // delay(10 * KLAPIC.ms_interval);

    for (int i = 0; i < 2; i++) {
        // write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        write_apic_register(APIC_ICRLO_OFF, (id << 32) | trampoline_page | ICR_STARTUP);
        // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
        delay(KLAPIC.ms_interval * 200 / 1000);
    }
    
}

void set_ap_ready(void) {
    ap_boot->ready = 1;
}

cpu_status_t *ipi_handler(cpu_status_t *status) {
    switch (KIPC.action) {
    case 1:
        load_cr3(PADDR(KIPC.addr));
        break;
    
    case 2:
        asm ("invlpg %0" : : "m" (KIPC.addr));
        break;

    case 3:
        log(Verbose, "SMP", "Test");
        break;
    }

    APIC_EOI();
    return status;
}
