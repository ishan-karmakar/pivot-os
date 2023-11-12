#include <cpu/lapic.h>
#include <kernel/logging.h>
#include <cpu/mp.h>
#include <cpu/idt.h>
#include <drivers/framebuffer.h>
#include <sys.h>
#include <mem/pmm.h>
#include <libc/string.h>

extern void ap_trampoline(void);
extern void *gdt64;
extern void *p4_table;
extern idtr_t idtr;

void start_ap(uint32_t, uint8_t);

volatile uint8_t aps_running;
ap_info_t *data = (ap_info_t*) VADDR(15 * PAGE_SIZE);

void start_aps(madt_t *madt) {
    data->gdt64 = (uintptr_t) &gdt64 - KERNEL_VIRTUAL_ADDR + 50;
    data->pml4 = (uintptr_t) &p4_table - KERNEL_VIRTUAL_ADDR;
    data->idtr = (uintptr_t) &idtr - KERNEL_VIRTUAL_ADDR;
    data->stack_top = (uintptr_t) alloc_frame();
    uint32_t current_apic_id = bsp_id();
    log(Verbose, "MP", "This processor's APIC ID is %u", current_apic_id);
    
    madt_item_t *lapic = get_madt_item(madt, MADT_LAPIC, 0);
    uint8_t count = 0;
    memcpy((void*) VADDR(0x8000), &ap_trampoline, 4096);
    while (lapic != NULL) {
        uint8_t apic_id = *((uint8_t*)(lapic + 1) + 1);
        if (apic_id != current_apic_id)
            start_ap(apic_id, 0x8);

        lapic = get_madt_item(madt, MADT_LAPIC, ++count);
    }
    while (aps_running < (count - 1)) asm ("pause");
    // printf("All processors started\n");
}

void start_ap(uint32_t id, uint8_t trampoline_page __attribute__((unused))) {
    // INIT
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");

    // INIT De assert
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_LEVEL);
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    mdelay(10);
    for (int i = 0; i < 2; i++) {
        write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
        while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
        udelay(200);
    }
    // log(Info, "MP", "Starting up processor %u", id);
}