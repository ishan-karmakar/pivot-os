#include <stdint.h>
#include <stddef.h>
#include <cpu/scheduler.h>
#include <cpu/lapic.h>
#include <mem/kheap.h>
#include <kernel/logging.h>

thread_t *root_thread = NULL;
thread_t *active_thread = NULL;

void add_thread(cpu_status_t *exec_frame) {
    thread_t *thread = kmalloc(sizeof(thread_t));
    log(Verbose, true, "SCHEDULER", "%x", exec_frame);
    thread->ef = exec_frame;
    thread->next = NULL;
    if (root_thread == NULL) {
        log(Verbose, true, "SCHEDULER", "This is the first thread, setting as active");
        root_thread = thread;
    } else {
        thread_t *current = root_thread;
        while (current->next != NULL)
            current = current->next;
        current->next = thread;
    }
    log(Verbose, true, "SCHEDULER", "Created task for function at address %x", exec_frame->rip);
}

void *get_next_thread(void) {
    if (active_thread == NULL || active_thread->next == NULL)
        return root_thread;
    else
        return active_thread->next;
}

void delete_thread(thread_t *thread) {
    if (thread == active_thread)
        active_thread = NULL;
    if (root_thread == thread) {
        root_thread = root_thread->next;
        kfree(thread->ef);
        kfree(thread);
        return;
    }
    thread_t *current = root_thread;
    thread_t *prev = current;
    while (current != thread) {
        prev = current;
        current = current->next;
    }
    prev->next = current->next;
    kfree(current->ef);
    kfree(current);
}

void thread_wrapper(void (*f)(void)) {
    f();
    delete_thread(active_thread);
    asm volatile ("int $35");
    // What to do when there are no threads left?
    // I think we should return back to the kernel_start function after the init_system call, but how to do it?
}