#include <stdint.h>
#include <stddef.h>
#include <cpu/scheduler.h>
#include <cpu/lapic.h>
#include <mem/kheap.h>
#include <kernel/logging.h>
#define NEXT_THREAD(thread) (thread->next == NULL ? root_thread : thread->next)

thread_t *root_thread = NULL;
thread_t *active_thread = NULL;
thread_t *failsafe_thread;
size_t main_thread_wakeup_time = 0;
extern cpu_status_t *create_thread_ef(void (*)(void), uintptr_t stack_pointer);
inline void scheduler_yield(void) { asm volatile ("int $35"); }

void failsafe_thread_fn(void) {
    while (1) asm ("pause");
}

void create_failsafe_thread(uintptr_t stack_pointer) {
    // RSP cannot be null, so we are putting in current stack pointer, but wait loop shouldn't do any stack operations
    cpu_status_t *exec_frame = create_thread_ef(&failsafe_thread_fn, stack_pointer);
    failsafe_thread = kmalloc(sizeof(thread_t));
    failsafe_thread->ef = exec_frame;
    failsafe_thread->wakeup_time = 0;
    failsafe_thread->next = NULL;
}

void create_thread(void (*fn)(void), uintptr_t stack_pointer) {
    cpu_status_t *exec_frame = create_thread_ef(fn, stack_pointer);
    thread_t *thread = kmalloc(sizeof(thread_t));
    thread->ef = exec_frame;
    thread->next = NULL;
    thread->wakeup_time = 0;
    if (root_thread == NULL)
        root_thread = thread;
    else {
        thread_t *current = root_thread;
        while (current->next != NULL)
            current = current->next;
        current->next = thread;
    }
    log(Verbose, true, "SCHEDULER", "Created task for function at address %x", exec_frame->rip);
}

// thread_t *next_thread(void) {
//     if (active_thread == NULL)
//         return NULL;
//     thread_t *thread = NEXT_THREAD(active_thread);
//     while (thread->wakeup_time > apic_ticks) {
//         log(Verbose, true, "THREAD", "%u - %u", thread->wakeup_time, apic_ticks);
//         if (thread == active_thread) {
//             // If code comes into this if clause, it has already failed the wakeup time condition
//             printf("Returning failsafe thread\n");
//             return failsafe_thread;
//         }
//         thread = NEXT_THREAD(thread);
//     }
//     return thread;
// }

thread_t *next_thread(void) {
    if (active_thread == NULL)
        return NULL;
    thread_t *thread = active_thread;
    while (true) {
        thread = NEXT_THREAD(thread);
        log(Verbose, true, "KERNEL", "%u - %u", thread->wakeup_time, apic_ticks);
        if (thread->wakeup_time <= apic_ticks) {
            log(Verbose, true, "KERNEL", "%x", thread);
            return thread;
        } else if (thread == active_thread) {
            log(Verbose, true, "KERNEL", "Using failsafe thread");
            return failsafe_thread;
        }
    }

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
    scheduler_yield();
}

void thread_sleep(size_t milliseconds) {
    active_thread->wakeup_time = apic_ticks + ((milliseconds * apic_ms_interval) / read_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF));
    scheduler_yield();
}