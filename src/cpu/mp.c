#include <cpu/lapic.h>
#include <kernel/logging.h>
#include <cpu/mp.h>

void start_ap(uint8_t id, uint8_t trampoline_page) {
    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_ASSERT | ICR_LEVEL);

    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING);
    log(Verbose, "LAPIC", "Processor received INIT");

    write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
    write_apic_register(APIC_ICRLO_OFF, ICR_INIT | ICR_LEVEL);
    
    while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING);
    log(Verbose, "LAPIC", "Processor received INIT Level De-assert");

    mdelay(10);
    for (int i = 0; i < 2; i++) {
        // write_apic_register(APIC_ICRHI_OFF, id << ICR_DEST_SHIFT);
        // write_apic_register(APIC_ICRLO_OFF, trampoline_page | ICR_STARTUP);
        // udelay(200);
        // while (read_apic_register(APIC_ICRLO_OFF) & ICR_SEND_PENDING);
    }
    log(Verbose, "LAPIC", "Processor received SIPI");
}