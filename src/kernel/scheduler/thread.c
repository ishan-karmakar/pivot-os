#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <libc/string.h>
#include <kernel/logging.h>

size_t next_thread_id = 0;

void thread_wrapper(void (*entry_point)(void));

thread_t *create_thread(char *name, thread_fn_t entry_point, struct task *parent_task, bool supervisor) {
    thread_t *thread = kmalloc(sizeof(thread_t));
    thread->id = next_thread_id++;
    thread->parent_task = parent_task;
    thread->status = NEW;
    thread->wakeup_time = 0;
    strcpy(thread->name, name);
    thread->next = NULL;
    thread->next_sibling = NULL;
    thread->ticks = 0;

    thread->ef = kmalloc(sizeof(cpu_status_t));
    thread->ef->int_no = 0x101;
    thread->ef->err_code = 0;

    thread->ef->rip = (uintptr_t) thread_wrapper;
    thread->ef->rdi = (uintptr_t) entry_point;

    thread->ef->rflags = 0x202;

    thread->ef->ss = 0x10;
    thread->ef->cs = 0x8;

    thread->rsp0 = (uintptr_t) valloc(THREAD_DEFAULT_STACK_SIZE, 0, &parent_task->vmm_info);
    thread->stack = (uintptr_t) valloc(THREAD_DEFAULT_STACK_SIZE, 0, &parent_task->vmm_info);
    thread->ef->rsp = thread->stack;
    thread->ef->rbp = 0;

    task_add_thread(parent_task, thread);
    scheduler_add_thread(thread);
    return thread;
}

void free_thread(thread_t *thread) {
    kfree(thread->ef);
    kfree((void*) (thread->stack - THREAD_DEFAULT_STACK_SIZE));
    kfree(thread);
}

void thread_wrapper(thread_fn_t entry_point) {
    entry_point();
    cur_thread->status = DEAD;
    while (1);
}

void idle(void) { while(1); }
