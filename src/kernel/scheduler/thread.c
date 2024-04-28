#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/heap.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <drivers/framebuffer.h>
#include <cpu/lapic.h>
#include <libc/string.h>
#include <kernel/logging.h>

size_t next_thread_id = 0;

void thread_wrapper(void (*entry_point)(void));
void init_thread_vmm(thread_t*);

thread_t *create_thread(char *name, thread_fn_t entry_point, bool add_scheduler_list) {
    thread_t *thread = malloc(sizeof(thread_t));
    thread->ef = malloc(sizeof(cpu_status_t));
    init_thread_vmm(thread);
    thread->id = next_thread_id++;
    thread->status = NEW;
    thread->wakeup_time = 0;
    strcpy(thread->name, name);
    thread->next = NULL;
    thread->ticks = 0;

    thread->ef->int_no = 0x101;
    thread->ef->err_code = 0;

    thread->ef->rip = (uintptr_t) thread_wrapper;
    thread->ef->rdi = (uintptr_t) entry_point;

    thread->ef->rflags = 0x202;

    thread->ef->ss = (4 * 8) | 3; // User data selector
    thread->ef->cs = (3 * 8) | 3; // User code selector

    thread->stack = (uintptr_t) valloc(THREAD_DEFAULT_STACK_SIZE, 0, &thread->vmm_info);
    thread->ef->rsp = thread->stack + THREAD_DEFAULT_STACK_SIZE; // Stack top
    thread->ef->rbp = 0;


    if (add_scheduler_list)
        scheduler_add_thread(thread);
    return thread;
}

void free_thread(thread_t *thread) {
    free(thread->ef);
    free((void*) (thread->stack - THREAD_DEFAULT_STACK_SIZE));
    free(thread);
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
    map_addr(get_phys_addr((uintptr_t) thread, NULL), (uintptr_t) thread, USER_PT_ENTRY, p4_tbl);

    init_heap(&thread->heap_info, &thread->vmm_info);
}

void thread_wrapper(thread_fn_t entry_point) {
    entry_point();
    // TODO: Move this to an interrupt so that cur_thread cannot be accessed by user thread
    cur_thread->status = DEAD;
    while (1);
}

void idle(void) { while(1); }
