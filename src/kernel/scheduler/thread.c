#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/heap.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <drivers/framebuffer.h>
#include <cpu/lapic.h>
#include <libc/string.h>
#include <kernel/logging.h>
#include <cpu/cpu.h>

size_t next_thread_id = 0;

void thread_wrapper(void (*entry_point)(void));
void init_thread_vmm(thread_t*);

thread_t *create_thread(char *name, thread_fn_t entry_point, bool safety, heap_info_t *hi) {
    thread_t *thread = halloc(sizeof(thread_t), hi);
    thread->ef = halloc(sizeof(cpu_status_t), hi);
    init_thread_vmm(thread);

    thread->id = next_thread_id++;
    thread->status = NEW;
    thread->wakeup_time = 0;
    strcpy(thread->name, name);
    thread->next = NULL;
    thread->ticks = 0;

    thread->ef->int_no = 0x101;
    thread->ef->err_code = 0;

    if (safety) {
        init_heap(&thread->heap_info, &thread->vmm_info);
        thread->ef->rip = (uintptr_t) thread_wrapper;
        thread->ef->rdi = (uintptr_t) entry_point;
    } else
        thread->ef->rip = (uintptr_t) entry_point;

    thread->ef->rflags = 0x202;

    thread->ef->ss = (4 * 8) | 3; // User data selector
    thread->ef->cs = (3 * 8) | 3; // User code selector

    thread->stack = (uintptr_t) valloc(THREAD_DEFAULT_STACK_SIZE, 0, &thread->vmm_info);
    thread->ef->rsp = thread->stack + THREAD_DEFAULT_STACK_SIZE; // Stack top
    thread->ef->rbp = 0;

    return thread;
}

void free_thread(thread_t *thread) {
    load_cr3(PADDR(mem_info->pml4));
    free_page_table(thread->vmm_info.p4_tbl, 4);
    vfree(thread->heap_info.heap_start);
    vfree(thread->ef);
    vfree((void*) thread->stack);
    vfree(thread);
}

void init_thread_vmm(thread_t *thread) {
    thread->vmm_info.p4_tbl = (page_table_t) VADDR(alloc_frame());
    page_table_t p4_tbl = thread->vmm_info.p4_tbl;
    clean_table(p4_tbl);
    map_addr(PADDR(p4_tbl), (uintptr_t) p4_tbl, KERNEL_PT_ENTRY, p4_tbl);
    map_addr(PADDR(p4_tbl), PADDR(p4_tbl), KERNEL_PT_ENTRY, p4_tbl);

    init_vmm(User, &thread->vmm_info);
    map_kernel_entries(p4_tbl);
    map_framebuffer(p4_tbl, USER_PT_ENTRY);
    map_lapic(p4_tbl);
    map_addr(get_phys_addr((uintptr_t) thread, NULL), (uintptr_t) thread, KERNEL_PT_ENTRY, p4_tbl);
}

void thread_wrapper(thread_fn_t entry_point) {
    entry_point();
    syscall(2, 0);
    scheduler_yield();
    while (1) asm ("pause");
}

void thread_sleep(size_t ms) {
    syscall(1, 1, ms);
    scheduler_yield();
}

void thread_sleep_syscall(cpu_status_t *status) {
    cur_thread->status = SLEEP;
    cur_thread->wakeup_time = apic_ticks + status->rdi;
}

void thread_dead_syscall(void) {
    cur_thread->status = DEAD;
}

void *malloc(size_t size) {
    return halloc(size, &cur_thread->heap_info);
}

void free(void *ptr) {
    return hfree(ptr, &cur_thread->heap_info);
}

void *realloc(void *ptr, size_t size) {
    return hrealloc(ptr, size, &cur_thread->heap_info);
}

void idle(void) { while(1) asm ("pause"); }
