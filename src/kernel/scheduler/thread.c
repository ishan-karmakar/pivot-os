#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/heap.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <drivers/framebuffer.h>
#include <cpu/lapic.h>
#include <libc/string.h>
#include <kernel/logging.h>
#include <io/stdio.h>
#include <cpu/cpu.h>
#include <kernel.h>

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
    vfree((void*) thread->stack, &thread->vmm_info);
    load_cr3(PADDR(KMEM.pml4));
    free_page_table(thread->vmm_info.p4_tbl, 4);
    free_heap(&thread->vmm_info, thread->heap);
}

void init_thread_vmm(thread_t *thread) {
    thread->vmm_info.p4_tbl = (page_table_t) VADDR(alloc_frame());
    clean_table(thread->vmm_info.p4_tbl);
    map_addr(PADDR(thread->vmm_info.p4_tbl), PADDR(thread->vmm_info.p4_tbl), KERNEL_PT_ENTRY, thread->vmm_info.p4_tbl);
    map_addr(PADDR(thread->vmm_info.p4_tbl), (uintptr_t) thread->vmm_info.p4_tbl, KERNEL_PT_ENTRY, thread->vmm_info.p4_tbl);
    map_kernel_entries(thread->vmm_info.p4_tbl);
    map_framebuffer(thread->vmm_info.p4_tbl, USER_PT_ENTRY);
    map_lapic(thread->vmm_info.p4_tbl);
    copy_heap(KHEAP, KMEM.pml4, thread->vmm_info.p4_tbl);
    map_pmm(thread->vmm_info.p4_tbl);
    load_cr3(PADDR(thread->vmm_info.p4_tbl));
    init_vmm(User, 32, &thread->vmm_info);
    thread->heap = heap_add(1, HEAP_DEFAULT_BS, &thread->vmm_info, NULL);
    thread->stack = (uintptr_t) valloc(THREAD_STACK_PAGES, &thread->vmm_info);
    load_cr3(PADDR(KMEM.pml4));
}

void thread_wrapper(thread_fn_t entry_point) {
    entry_point();
    syscall(2, 0);
    thread_yield();
    while (1) asm ("pause");
}

void *malloc(size_t size) {
    return halloc(size, KSCHED.cur->heap);
}

void free(void *ptr) {
    return hfree(ptr, KSCHED.cur->heap);
}

void *realloc(void *ptr, size_t size) {
    return hrealloc(ptr, size, *KSCHED.cur->heap);
}

void idle(void) { while(1) asm ("pause"); }

void thread_sleep(size_t ms) {
    syscall(1, 1, ms);
    thread_yield();
}

void thread_yield(void) {
    asm volatile ("int $0x20");
}

void thread_sleep_syscall(cpu_status_t *status) {
    KSCHED.cur->status = SLEEP;
    KSCHED.cur->wakeup_time = KLAPIC.apic_ticks + status->rdi;
}

void thread_dead_syscall(void) {
    KSCHED.cur->status = DEAD;
}