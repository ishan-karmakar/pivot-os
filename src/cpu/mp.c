#include <cpu/lapic.h>
#include <kernel/logging.h>
#include <cpu/mp.h>
#include <drivers/framebuffer.h>
#include <sys.h>
#include <libc/string.h>
extern void ap_trampoline(void);
void start_ap(uint8_t, uint8_t);
uint8_t apsrunning;
void start_aps(madt_t *madt) {
    uint32_t current_apic_id = bsp_id();
    log(Verbose, "MP", "This processor's APIC ID is %u", current_apic_id);
    
    madt_item_t *lapic = get_madt_item(madt, MADT_LAPIC, 0);
    uint8_t count = 0;
    memcpy((void*) VADDR(0x8000), &ap_trampoline, PAGE_SIZE);
    while (lapic != NULL) {
        uint8_t apic_id = *((uint8_t*)(lapic + 1) + 1);
        if (apic_id != current_apic_id) {
            log(Info, "MP", "Starting up processor %u", apic_id);
            start_ap(apic_id, 0x8);
        }

        lapic = get_madt_item(madt, MADT_LAPIC, ++count);
    }
}

void start_ap(uint8_t id, uint8_t trampoline_page) {
    // *(uint8_t*) VADDR(16 * PAGE_SIZE) = 0;
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);

    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    log(Verbose, "MP", "Processor received INIT");
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_LEVEL);

    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    log(Verbose, "MP", "Processor received INIT Level De-assert");
    mdelay(10);
    for (int i = 0; i < 2; i++) {
        write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
        udelay(200);
        while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    }
    log(Verbose, "MP", "Processor received SIPI");
    // log(Verbose, "LAPIC", "%u", (uint8_t)(*(uint8_t*) VADDR(16 * PAGE_SIZE)));
}