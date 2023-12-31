#include <stdint.h>
#include <stddef.h>
#include <cpu/scheduler.h>
#include <cpu/lapic.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <kernel/logging.h>
#include <io/stdio.h>
#include <drivers/qemu.h>

#define VALID_THREAD(thread) (!thread->wakeup_time && !thread->ended)
#define SCHEDULER_TICK_INTERVAL 5

thread_t *root_thread = NULL; // Root thread in thread list
thread_t *last_thread = NULL; // Last thread that ran, could be from thread list or failsafe thread
thread_t *active_thread = NULL; // Active thread IN thread list
thread_t *failsafe_thread;
// Tells assembly how to load thread
// 0 - Don't load or save EF (same thread)
// 1 - Don't load, only save EF (first thread)
// 2 - Save and load EF (all other threads)
uint8_t load_type;
size_t scheduler_ticks = 0; // Copy of apic_ticks but only scheduler is changing it

extern cpu_status_t *create_thread_ef(void (*)(void), uintptr_t stack_pointer, bool);
extern void failsafe_thread_fn(void);
extern void scheduler_yield(void);

void create_failsafe_thread(void) {
    // RSP cannot be null, so we are putting in current stack pointer, but wait loop shouldn't do any stack operations
    register uintptr_t stack_pointer asm ("sp");
    cpu_status_t *exec_frame = create_thread_ef(&failsafe_thread_fn, stack_pointer, false);
    failsafe_thread = kmalloc(sizeof(thread_t));
    failsafe_thread->ef = exec_frame;
    failsafe_thread->wakeup_time = 0;
    failsafe_thread->next = NULL;
    failsafe_thread->ended = false;
}

void create_thread(thread_fn_t fn, uintptr_t stack_pointer) {
    cpu_status_t *exec_frame = create_thread_ef(fn, stack_pointer, true);
    thread_t *thread = kmalloc(sizeof(thread_t));
    thread->ef = exec_frame;
    thread->next = NULL;
    thread->prev = NULL;
    thread->wakeup_time = 0;
    thread->ended = false;
    if (root_thread == NULL) {
        root_thread = thread;
        root_thread->next = root_thread;
    } else {
        thread_t *current = root_thread;
        while (current->next != root_thread)
            current = current->next;
        thread->next = current->next;
        current->next = thread;
        thread->prev = current;
    }
    log(Verbose, "SCHEDULER", "Created task at address %x", thread);
}

void print_threads(void) {
    thread_t *cur = root_thread;
    size_t i = 0;
    while (1) {
        log(Info, "SCHEDULER", "[%u] Thread address: %x, EF address: %x, Stack top: %x", i++, (uintptr_t) cur, (uintptr_t) cur->ef, cur->ef->rsp);
        if (cur->next == root_thread)
            break;
        cur = cur->next;
    }
}

// Check if any threads are scheduled to wakeup now
// TODO: Figure out more efficient way to implement this
// Right now has to traverse list once for wakeup threads,
// then another time if there are no wakeup threads
thread_t *check_wakeup_threads(void) {
    thread_t *cur = root_thread;
    while (1) {
        if (cur->wakeup_time <= apic_ticks && cur->wakeup_time)
            return cur;
        cur = cur->next;
        if (cur == root_thread)
            return NULL;        
    }
}

thread_t *next_thread(void) {
    scheduler_ticks++;
    thread_t *cur;
    // If active thread has ended, switch tasks
    // If active thread start sleeping, switch tasks
    // printf("-\n");
    thread_t *wakeup_thread = check_wakeup_threads();
    if (wakeup_thread) {
        wakeup_thread->wakeup_time = 0;
        active_thread = wakeup_thread;
        scheduler_ticks = 0;
        load_type = 2;
        printf("S%x\n", wakeup_thread);
        return wakeup_thread;
        // Not time to switch preempt tasks
        // Check whether any other task wakes up now
        // printf("C\n");
    } else if (scheduler_ticks % (50 / SCHEDULER_TICK_INTERVAL) && VALID_THREAD(active_thread)) {
        load_type = 0;
        return NULL;
    }
    scheduler_ticks = 0;
    // Check if no thread has been run yet
    if (last_thread == NULL) {
        active_thread = root_thread;
        load_type = 1; // DON'T SAVE EF, ONLY LOAD EF
        return active_thread;
    }

    thread_t *start = active_thread->next;
    if (active_thread->ended && active_thread == last_thread) {
        // Active thread has ended AND ran last quantum
        active_thread->prev->next = active_thread->next;
        active_thread->next->prev = active_thread->prev;
        kfree(active_thread->ef);
        kfree(active_thread);
        active_thread = NULL;
        last_thread = NULL;
    }

    cur = start;
    while (1) {
        if (VALID_THREAD(cur)) {
            log(Verbose, "SCHEDULER", "%x is valid thread, last thread was %x", cur, last_thread);
            if (cur != last_thread) {
                load_type = 2;
                active_thread = cur;
                return cur;
            } else
                break;
        }
        cur = cur->next;
        if (cur == start) {
            if (failsafe_thread != last_thread) {
                load_type = 2;
                return failsafe_thread;
            } else
                break;
        }
    }
    load_type = 0;
    return NULL;
}

void thread_wrapper(void (*f)(void)) {
    f();
    active_thread->ended = true;
    apic_ticks--;
    scheduler_yield();
    while(1);
}

void thread_sleep(size_t milliseconds) {
    active_thread->wakeup_time = apic_ticks + (milliseconds / SCHEDULER_TICK_INTERVAL);
    // Decrementing apic_ticks because as a side effect of triggering the interrupt, apic_ticks++'s even though it wasn't a "timer interrupt"
    // No need to decrement scheduler_ticks because it will be reset to 0 inside next_thread
    apic_ticks--;
    scheduler_yield();
}

void init_scheduler(thread_fn_t kernel_fn) {
    create_failsafe_thread();
    create_thread(kernel_fn, VADDR((uintptr_t) alloc_frame()) + PAGE_SIZE);
    char_printer = qemu_write_char;
}

void start_scheduler(void) {
    // Want it to become 0 at the first tick,
    // so I set it to the max value so it will overflow and loop back to 0
    apic_ticks = scheduler_ticks = (uint64_t) - 1;
    start_apic_timer(APIC_TIMER_PERIODIC, SCHEDULER_TICK_INTERVAL * apic_ms_interval, APIC_TIMER_PERIODIC_IDT_ENTRY);
}

void testt(uintptr_t test) {
    log(Verbose, "SCHEDULER", "%x", test);
}