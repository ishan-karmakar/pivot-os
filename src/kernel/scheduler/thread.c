#include <scheduler/thread.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <libc/string.h>

thread_t *create_thread(char *name, void (*entry_point)(void)) {
    // thread_t *new_thread = kmalloc(sizeof(thread_t));
    // new_thread->id = next_thread_id++;
    // new_thread->status = NEW;
    // new_thread->wakeup_time = 0;
    // strcpy(new_thread->name, name);
    // new_thread->next = NULL;
    // new_thread->next_sibling = NULL;
    // new_thread->ticks = 0;

    // new_thread->ef = kmalloc(sizeof(cpu_status_t));
    // new_thread->ef->int_no = 0x101;
    // new_thread->ef->err_code = 0;

    // new_thread->ef->rip = thread_wrapper;
    // new_thread->ef->rdi = (uintptr_t) entry_point;

    // new_thread->ef->rflags = 0x202;
    // new_thread->ef->ss = 0x10;
    // new_thread->ef->cs = 0x08;

    // void *stack_pointer = alloc_range(2)
}