#include <kernel.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/tss.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <cpu/smp.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <mem/vmm.h>
#include <drivers/qemu.h>
#include <drivers/framebuffer.h>
#include <drivers/keyboard.h>
#include <acpi/acpi.h>
#include <kernel/rtc.h>
#include <kernel/logging.h>
#include <kernel/progress.h>
#include <io/stdio.h>

kernel_info_t kinfo;

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
    thread_sleep(2000);
    printf("Hello World after thread_sleep\n");
}

void task3(void) {
    printf("Hello World 3\n");
    thread_sleep(1000);
    printf("Hello World after thread_sleep3\n");
}

void user_function(void) {
    while(1);
}

// TODO: Support booting with BIOS and UEFI
void __attribute__((noreturn)) init_kernel(kernel_info_t *kernel_info, uintptr_t stack) {
    CPU = 0;
    kinfo = *kernel_info; // Copy over boot info to higher half
    init_qemu();
    init_gdt();
    init_idt();
    IDT_SET_TRAP(SYSCALL_IDT_ENTRY, 3, syscall_irq);
    IDT_SET_INT(IPI_IDT_ENTRY, 0, ipi_irq);
    init_pmm();
    init_framebuffer();
    init_vmm(Supervisor, KMEM.mem_pages, &KVMM);
    KHEAP = heap_add(1, HEAP_DEFAULT_BS, &KVMM, NULL);
    init_acpi();
    init_tss();
    set_rsp0();
    init_lapic();
    init_ioapic();
    calibrate_apic_timer();
    init_rtc();
    init_keyboard();
    // clear_screen();
    KCPUS[CPU].stack = stack;
    KSMP.idle = create_thread("idle", idle, false);
    start_aps();
    // scheduler_add_thread(create_thread("test1", task1, true));
    // scheduler_add_thread(create_thread("test3", task3, true));
    // scheduler_add_thread(create_thread("test2", task2, true));
    // start_apic_timer(APIC_TIMER_PERIODIC, KLAPIC.ms_interval * 50, APIC_PERIODIC_IDT_ENTRY);
    while (1);
}

void __attribute__((noreturn)) kernel_main(void) {
    // Kernel should now be completely initialized
    while (1);
}