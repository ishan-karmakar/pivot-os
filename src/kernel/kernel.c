#include <boot.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <mem/vmm.h>
#include <drivers/qemu.h>
#include <drivers/framebuffer.h>
#include <kernel/acpi.h>
#include <kernel/rtc.h>
#include <kernel/logging.h>
#include <kernel/progress.h>
#include <io/stdio.h>

log_level_t min_log_level = Debug;
boot_info_t boot_info;
heap_info_t kheap_info;

void __attribute__((noreturn)) kernel_main(void);

void __attribute__((noreturn)) hcf(void) {
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}

void task1(void) {
    printf("Hello World\n");
}

void task2(void) {
    printf("Hello World 2\n");
    thread_sleep(500);
    printf("Hello World after thread_sleep\n");
}

void user_function(void) {
    while(1);
}

void __attribute__((noreturn)) init_kernel(boot_info_t *binfo) {
    boot_info = *binfo; // Copy over boot info to higher half
    init_qemu();
    init_tss();
    init_gdt();
    init_idt();
    init_pmm(&boot_info.mem_info);
    init_acpi(&boot_info);
    init_framebuffer(&boot_info.fb_info);
    // while(1);
    init_vmm(Supervisor, NULL);
    init_heap(&kheap_info, NULL);
    set_heap(&kheap_info);
    init_lapic();
    init_ioapic();
    calibrate_apic_timer();
    init_rtc();
    // clear_screen();
    idle_thread = create_thread("idle", idle, false);
    create_thread("test1", task1, true);
    create_thread("test2", task2, true);
    printf("\n");
    uintptr_t rsp;
    asm volatile ("mov %%rsp, %0" : "=r" (rsp));
    set_rsp0(rsp);
    start_apic_timer(APIC_TIMER_PERIODIC, apic_ms_interval, APIC_TIMER_PERIODIC_IDT_ENTRY);
    while (1);
}

void __attribute__((noreturn)) kernel_main(void) {
    // Kernel should now be completely initialized
    while (1);
}