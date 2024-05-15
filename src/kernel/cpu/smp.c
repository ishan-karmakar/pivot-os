#include <sys.h>
#include <cpu/smp.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/lapic.h>
#include <cpu/cpu.h>
#include <mem/pmm.h>
#include <mem/bitmap.h>
#include <kernel/logging.h>
#include <libc/string.h>

extern void ap_trampoline(void);
extern gdtr_t gdtr;
extern idtr_t idtr;

void start_ap(uint32_t, uint8_t);

volatile ap_info_t *ap_info = (ap_info_t*) 0x8010;

void start_aps(void) {
    map_addr(0x8000, 0x8000, KERNEL_PT_ENTRY, NULL);
    memcpy((void*) 0x8000, &ap_trampoline, PAGE_SIZE);
    ap_info->gdtr = (uintptr_t) &gdtr;
    ap_info->idtr = (uintptr_t) &idtr;
    ap_info->pml4 = PADDR(mem_info->pml4);
    madt_t *madt = (madt_t*) get_table("APIC");
    uint32_t cur_id = get_apic_id();
    log(Verbose, "SMP", "Current processor id: %u", cur_id);

    madt_item_t *lapic = get_madt_item(madt, MADT_LAPIC, 0);
    uint8_t count = 0;

    while (lapic != NULL) {
        uint8_t apic_id = *((uint8_t*) (lapic + 1) + 1);
        if (apic_id != cur_id) {
            ap_info->stack_top = (uintptr_t) alloc_frame() + PAGE_SIZE;
            map_addr(ap_info->stack_top - PAGE_SIZE, ap_info->stack_top - PAGE_SIZE, KERNEL_PT_ENTRY, NULL);
            start_ap(apic_id, 0x8);
            while (!ap_info->ready) asm ("pause");
            bitmap_clear_bit(ap_info->stack_top - PAGE_SIZE);
        }
        
        lapic = get_madt_item(madt, MADT_LAPIC, ++count);
    }

    bitmap_clear_bit(0x8000);
    log(Info, "SMP", "All CPUs booted up");
}

void start_ap(uint32_t id, uint8_t trampoline_page) {
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    delay(10 * apic_ms_interval);

    for (int i = 0; i < 2; i++) {
        write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
        while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
        delay(apic_ms_interval * 200 / 1000);
    }
    
}

void set_ap_ready(void) {
    ap_info->ready = 1;
}
