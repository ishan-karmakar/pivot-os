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
extern cpu_status_t *create_thread_ef(void (*)(void), uintptr_t stack_pointer, bool);
thread_status_t get_thread_status(void) { return active_thread->status; }
void set_thread_status(thread_status_t s) { active_thread->status = s; }
void scheduler_yield(void) {
    asm volatile ("int $35");
}

void failsafe_thread_fn(void) {
    while (1) asm ("pause");
}

void create_failsafe_thread(uintptr_t stack_pointer) {
    // TODO: stop failsafe thread from using thread wrapper, because we know it won't end
    // RSP cannot be null, so we are putting in current stack pointer, but wait loop shouldn't do any stack operations
    cpu_status_t *exec_frame = create_thread_ef(&failsafe_thread_fn, stack_pointer, false);
    failsafe_thread = kmalloc(sizeof(thread_t));
    failsafe_thread->ef = exec_frame;
    failsafe_thread->wakeup_time = 0;
    failsafe_thread->next = NULL;
}

void create_thread(void (*fn)(void), uintptr_t stack_pointer) {
    cpu_status_t *exec_frame = create_thread_ef(fn, stack_pointer, true);
    thread_t *thread = kmalloc(sizeof(thread_t));
    thread->ef = exec_frame;
    thread->next = NULL;
    thread->prev = NULL;
    thread->wakeup_time = 0;
    thread->status = STOPPED;
    if (root_thread == NULL)
        root_thread = thread;
    else {
        thread_t *current = root_thread;
        while (current->next != NULL)
            current = current->next;
        current->next = thread;
        thread->prev = current;
    }
    log(Verbose, true, "SCHEDULER", "Created task at address %x", thread);
}

thread_t *next_thread(void) {
    if (active_thread == NULL)
        return NULL;

    thread_t *thread = active_thread;
    switch (thread->status) {
    case ENDED:
        if (thread->prev)
            thread->prev->next = thread->next;

        if (thread->next)
            thread->next->prev = thread->prev;

        thread = NEXT_THREAD(thread);
        kfree(thread->ef);
        kfree(thread);
        break;
    case RUNNING:
        thread->status = STOPPED;
        // FALLTHRU
    default:
        thread = NEXT_THREAD(thread);
    }
    thread_t *org_thread = thread;
    while (true) {
        if (thread->wakeup_time <= apic_ticks) {
            thread->status = RUNNING;
            return thread;
        } else if (thread == org_thread) {
            return failsafe_thread;
        }
        thread = NEXT_THREAD(thread);
    }
}

void thread_wrapper(void (*f)(void)) {
    f();
    active_thread->status = ENDED;
    scheduler_yield();
}

void thread_sleep(size_t milliseconds) {
    active_thread->wakeup_time = apic_ticks + ((milliseconds * apic_ms_interval) / read_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF));
    scheduler_yield();
}