#include <kernel/logging.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/idt.h>
#include <cpu/smp.h>

void ap_kernel(void) {
    init_tss();
    init_lapic_ap();
    set_ap_ready();
    // No need to calibrate APIC timer again, will run at very similar frequency
    while(1);
}