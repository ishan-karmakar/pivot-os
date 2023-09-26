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
