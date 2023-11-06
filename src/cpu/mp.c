#include <cpu/lapic.h>
#include <kernel/logging.h>
#include <cpu/mp.h>
#include <drivers/framebuffer.h>
#include <sys.h>
#include <libc/string.h>
extern void ap_trampoline(void);
void start_ap(uint32_t, uint8_t);
uint8_t apsrunning;
void start_aps(madt_t *madt) {
    uint32_t current_apic_id = bsp_id();
    log(Verbose, "MP", "This processor's APIC ID is %u", current_apic_id);
    
    madt_item_t *lapic = get_madt_item(madt, MADT_LAPIC, 0);
    uint8_t count = 0;
    // memcpy((void*) VADDR(0x8000), &ap_trampoline, PAGE_SIZE);
    while (lapic != NULL) {
        uint8_t apic_id = *((uint8_t*)(lapic + 1) + 1);
        if (apic_id != current_apic_id) {
            log(Info, "MP", "Starting up processor %u", apic_id);
            // start_ap(apic_id, 0x8);
        }

        lapic = get_madt_item(madt, MADT_LAPIC, ++count);
    }
}

void start_ap(uint32_t id, uint8_t trampoline_page __attribute__((unused))) {
    volatile uint8_t *processor_status = (volatile uint8_t*) VADDR(16 * PAGE_SIZE);
    *processor_status = 0;
    // printf("a\n");
    log(Verbose, "MP", "Sending INIT to processor");
    // write_apic_register(APIC_ICRHI_OFF, id << 24); // This is the problem
    // write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);

    // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    // mdelay(10);
    // write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    // write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
    // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING) asm ("pause");
    // log(Verbose, "MP", "Processor received SIPI");
    // while (*processor_status != 2) asm ("pause");
    // log(Verbose, "MP", "Processor started");

}