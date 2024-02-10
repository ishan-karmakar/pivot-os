#include <boot.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <scheduler/task.h>
#include <mem/pmm.h>
#include <mem/kheap.h>
#include <mem/vmm.h>
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

void __attribute__((noreturn)) init_kernel(boot_info_t *boot_info) {
    init_qemu();
    init_tss();
    init_gdt();
    init_idt();
    init_pmm(&boot_info->mem_info);
    init_framebuffer(&boot_info->fb_info);
    map_phys_mem();
    init_vmm(Supervisor, NULL);
    init_kheap();
    init_acpi(boot_info);
    init_lapic();
    init_ioapic();
    calibrate_apic_timer();
    init_rtc();

    task_t *task1 = create_task("task1", task1, true);

    // init_scheduler();
    while (1);
}

void __attribute__((noreturn)) kernel_main(void) {
    // Kernel should now be completely initialized
    while (1);
}