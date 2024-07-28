#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/heap.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <drivers/framebuffer.h>
#include <cpu/lapic.h>
#include <libc/string.h>
#include <util/logger.hpp>
#include <io/stdio.h>
#include <cpu/cpu.h>
#include <cpu/idt.h>
#include <cpu/smp.h>
#include <kernel.h>
#include <common.h>

void thread_wrapper(thread_fn_t);
void init_thread_vmm(thread_t*);

thread_t *create_thread(char *name, thread_fn_t entry_point, bool safety) {
    thread_t *thread = halloc(sizeof(thread_t), KHEAP);
    thread->ef = halloc(sizeof(cpu_status_t), KHEAP);
    init_thread_vmm(thread);

    thread->status = NEW;
    thread->wakeup_time = 0;
    strcpy(thread->name, name);
    thread->next = NULL;
    thread->ticks = 0;

    thread->ef->int_no = 0x101;
    thread->ef->err_code = 0;

    if (safety) {
        thread->ef->rip = (uintptr_t) thread_wrapper;
        thread->ef->rdi = (uintptr_t) entry_point;
    } else
        thread->ef->rip = (uintptr_t) entry_point;

    thread->ef->rflags = 0x202;

    thread->ef->ss = (4 * 8) | 3; // User data selector
    thread->ef->cs = (3 * 8) | 3; // User code selector

    thread->ef->rsp = thread->stack + THREAD_STACK_PAGES * PAGE_SIZE; // Stack top
    thread->ef->rbp = 0;
    log(Verbose,  "THREAD", "Created %s thread", thread->name);
    return thread;
}

void free_thread(thread_t *thread) {
    vfree((void*) thread->stack, &thread->vmm);
    load_cr3(PADDR(KMEM.pml4));
    free_page_table(thread->vmm.p4_tbl, 4);
    free_heap(&thread->vmm, thread->heap);
}

void init_thread_vmm(thread_t *thread) {
    thread->vmm.p4_tbl = (page_table_t) alloc_frame();
    clean_table(thread->vmm.p4_tbl);
    map_addr((uintptr_t) thread->vmm.p4_tbl, (uintptr_t) thread->vmm.p4_tbl, KERNEL_PT_ENTRY, thread->vmm.p4_tbl);
    map_addr(KCPUS[CPU].stack, VADDR(KCPUS[CPU].stack), KERNEL_PT_ENTRY, thread->vmm.p4_tbl);
    map_kernel_entries(thread->vmm.p4_tbl);
    map_framebuffer(thread->vmm.p4_tbl, USER_PT_ENTRY);
    map_lapic(thread->vmm.p4_tbl);
    map_heap(KHEAP, KMEM.pml4, thread->vmm.p4_tbl);
    map_pmm(thread->vmm.p4_tbl);
    load_cr3((uintptr_t) thread->vmm.p4_tbl);
    init_vmm(User, 32, &thread->vmm);
    thread->heap = heap_add(1, HEAP_DEFAULT_BS, &thread->vmm, NULL);
    thread->stack = (uintptr_t) valloc(THREAD_STACK_PAGES, &thread->vmm);
    load_cr3(PADDR(KMEM.pml4));
}

void thread_wrapper(thread_fn_t entry_point) {
    entry_point();
    syscall(2, 0);
    thread_yield();
    while (1) asm ("pause");
}

void *malloc(size_t size) {
    return halloc(size, KCPUS[CPU].cur->heap);
}

void free(void *ptr) {
    return hfree(ptr, KCPUS[CPU].cur->heap);
}

void *realloc(void *ptr, size_t size) {
    return hrealloc(ptr, size, KCPUS[CPU].cur->heap);
}

void idle(void) { while(1) asm ("pause"); }

void thread_sleep(size_t ms) {
    syscall(1, 1, ms);
    thread_yield();
}

void thread_yield(void) {
    asm volatile ("int %0" : : "n" (APIC_PERIODIC_IDT_ENTRY));
}

void thread_sleep_syscall(cpu_status_t *status) {
    KCPUS[CPU].cur->status = SLEEP;
    KCPUS[CPU].cur->wakeup_time = KCPUS[CPU].ticks + status->rdi;
}

void thread_dead_syscall(void) {
    KCPUS[CPU].cur->status = DEAD;
}