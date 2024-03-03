#include <boot.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <scheduler/task.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/pmm.h>
#include <mem/kheap.h>
#include <mem/vmm.h>
#include <drivers/qemu.h>
#include <drivers/framebuffer.h>
#include <kernel/acpi.h>
#include <kernel/rtc.h>
#include <kernel/logging.h>
#include <io/stdio.h>

log_level_t min_log_level = Verbose;
boot_info_t boot_info;

void __attribute__((noreturn)) kernel_main(void);

void __attribute__((noreturn)) hcf(void) {
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}

void task1(void) {
    printf("Hello World\n");
}

void __attribute__((noreturn)) init_kernel(boot_info_t *binfo) {
    boot_info = *binfo; // Copy over boot info to higher half
    init_qemu();
    init_tss();
    init_gdt();
    init_idt();
    init_pmm(&boot_info.mem_info);
    init_framebuffer(&boot_info.fb_info);
    init_vmm(Supervisor, NULL);
    init_kheap();
    init_acpi(&boot_info);
    init_lapic();
    init_ioapic();
    calibrate_apic_timer();
    init_rtc();
    // clear_screen();
    // task_t *idle_task = create_task("idle", idle, true, false);
    // create_task("test1", task1, true, true);
    // idle_thread = idle_task->threads;

    // start_apic_timer(APIC_TIMER_PERIODIC, apic_ms_interval, APIC_TIMER_PERIODIC_IDT_ENTRY);
    while (1);
}

void __attribute__((noreturn)) kernel_main(void) {
    // Kernel should now be completely initialized
    while (1);
}