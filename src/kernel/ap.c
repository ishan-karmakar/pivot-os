#include <util/logger.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/idt.h>
#include <cpu/smp.h>
#include <cpu/cpu.h>
#include <scheduler/thread.h>
#include <mem/heap.h>
#include <kernel.h>
#include <io/stdio.h>
#include <drivers/qemu.h>

void ap_kernel(void) {
    // No need to calibrate APIC timer again, will run at very similar frequency
    init_lapic_ap();
    init_tss();
    set_rsp0();
    start_apic_timer(APIC_TIMER_PERIODIC, KLAPIC.ms_interval * 50, APIC_PERIODIC_IDT_ENTRY);
    // set_ap_ready();
    while(1);
}