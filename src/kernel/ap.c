#include <kernel/logging.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/idt.h>
#include <cpu/smp.h>
#include <mem/heap.h>

void ap_kernel(void) {
    heap_region_t heap_info;
    init_lapic_ap();
    // No need to calibrate APIC timer again, will run at very similar frequency
    // init_heap(&heap_info, NULL);
    init_tss(&heap_info);
    set_ap_ready();
    while(1);
}