#include <sys.h>
#include <cpu/smp.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/lapic.h>
#include <mem/pmm.h>
#include <kernel/logging.h>
#include <libc/string.h>

extern void ap_trampoline(void);
extern gdtr_t gdtr;
extern idtr_t idtr;

void start_ap(uint32_t, uint8_t);

ap_info_t *data = (ap_info_t*) 0x9000;
volatile uint8_t aps_running = 0;

void start_aps(void) {
    // TODO: Directly inject GDTR, IDTR, and PML4 into data section in ap_trampoline
    map_addr(0x8000, 0x8000, KERNEL_PT_ENTRY, NULL);
    map_addr(0x9000, 0x9000, KERNEL_PT_ENTRY, NULL);
    data->gdtr = (uintptr_t) &gdtr;
    data->idtr = (uintptr_t) &idtr;
    data->pml4 = PADDR(mem_info->pml4);
    madt_t *madt = (madt_t*) get_table("APIC");
    uint32_t cur_id = get_apic_id();
    log(Verbose, "SMP", "Current processor id: %u", cur_id);

    madt_item_t *lapic = get_madt_item(madt, MADT_LAPIC, 0);
    uint8_t count = 0;

    memcpy((void*) 0x8000, &ap_trampoline, PAGE_SIZE);
    while (lapic != NULL) {
        uint8_t apic_id = *((uint8_t*) (lapic + 1) + 1);
        if (apic_id != cur_id) {
            data->stack_top = (uintptr_t) alloc_frame() + PAGE_SIZE;
            map_addr(data->stack_top - PAGE_SIZE, data->stack_top - PAGE_SIZE, KERNEL_PT_ENTRY, NULL);
            start_ap(apic_id, 0x8);
            uint8_t old_aps_running = aps_running;
            while (aps_running == old_aps_running) asm ("pause");
            unmap_addr(data->stack_top - PAGE_SIZE, NULL);
            // UNMAP stack_top
        }
        
        lapic = get_madt_item(madt, MADT_LAPIC, ++count);
    }

}

void start_ap(uint32_t id, uint8_t trampoline_page) {
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    delay(10 * apic_ms_interval);

    for (int i = 0; i < 1; i++) {
        write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
        while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
        delay(apic_ms_interval * 200 / 1000);
    }
    
}