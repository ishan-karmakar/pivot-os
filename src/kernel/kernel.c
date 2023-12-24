#include <boot.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/scheduler.h>
#include <mem/pmm.h>
#include <mem/kheap.h>
#include <drivers/qemu.h>
#include <drivers/framebuffer.h>
#include <kernel/acpi.h>
#include <kernel/rtc.h>
#include <io/stdio.h>

void __attribute__((noreturn)) kernel_main(void);

void __attribute__((noreturn)) hcf(void) {
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}

void __attribute__((optimize("O0"))) task1(void) {
    printf("_\n");
}

void __attribute__((optimize("O0"))) task2(void) {
    printf("!\n");
}

void __attribute__((optimize("O0"))) task3(void) {
    printf(";\n");
}

void __attribute__((noreturn)) init_kernel(boot_info_t *boot_info) {
    init_qemu();
    init_gdt();
    init_idt();
    init_pmm(boot_info);
    init_framebuffer(boot_info);
    map_phys_mem();
    init_kheap();
    init_acpi(boot_info);
    init_lapic();
    init_ioapic();
    calibrate_apic_timer();
    init_rtc();

    create_failsafe_thread();
    create_thread(kernel_main, VADDR((uintptr_t) alloc_frame()));
    create_thread(task1, VADDR((uintptr_t) alloc_frame()));
    create_thread(task2, VADDR((uintptr_t) alloc_frame()));
    create_thread(task3, VADDR((uintptr_t) alloc_frame()));

    start_apic_timer(APIC_TIMER_PERIODIC, 50 * apic_ms_interval, APIC_TIMER_PERIODIC_IDT_ENTRY);
    while (1);
}

void __attribute__((noreturn)) kernel_main(void) {
    // Kernel should now be completely initialized
    while (1);
}