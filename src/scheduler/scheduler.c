#include <stddef.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <acpi/madt.h>
#include <cpu/tss.h>
#include <libc/string.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <util/logger.hpp>
#include <io/stdio.h>
#include <cpu/smp.h>
#include <kernel.h>
#include <kernel.hpp>

thread_t *scheduler_next_thread(thread_t*);
void scheduler_remove_thread(thread_t * volatile*, thread_t*);
cpu_status_t *ready_thread(thread_t*, bool);

cpu_status_t *schedule(cpu_status_t *cur_status) {
    log(VERBOSE, "SMP", "%p", read_apic_register(APIC_TIMER_LVT_OFFSET));
    return ready_thread(KSMP.idle, false);
    if (!KCPUS[CPU].threads && !KCPUS[CPU].wakeups)
        return ready_thread(KSMP.idle, false); // CR3 will already be set to idle_thread's

    if (KCPUS[CPU].cur != NULL) {
        if ((KCPUS[CPU].wakeups && KCPUS[CPU].wakeups->wakeup_time > KCPUS[CPU].ticks) &&
            KCPUS[CPU].cur->ticks++ < SCHEDULER_THREAD_TICKS &&
            KCPUS[CPU].cur->status != DEAD && KCPUS[CPU].cur->status != SLEEP)
            return cur_status;

        if (KCPUS[CPU].cur->status != DEAD && KCPUS[CPU].cur->status != SLEEP)
            KCPUS[CPU].cur->status = READY;

        memcpy(KCPUS[CPU].cur->ef, cur_status, sizeof(cpu_status_t));

        if (KCPUS[CPU].cur->status == DEAD) {
            scheduler_remove_thread(&KCPUS[CPU].threads, KCPUS[CPU].cur);
            KCPUS[CPU].cur = NULL;
        } else if (KCPUS[CPU].cur->status == SLEEP) {
            thread_t *thread = KCPUS[CPU].cur;
            KCPUS[CPU].cur = NULL;
            scheduler_remove_thread(&KCPUS[CPU].threads, thread);
            thread_t *prev = NULL;
            thread_t *item = KCPUS[CPU].wakeups;
            while (item != NULL && item->wakeup_time < thread->wakeup_time) {
                prev = item;
                item = item->next;
            }
            if (prev)
                prev->next = thread;
            else
                KCPUS[CPU].wakeups = thread;
            thread->next = item;
        }
    }

    if (KCPUS[CPU].wakeups && KCPUS[CPU].wakeups->wakeup_time <= KCPUS[CPU].ticks) {
        thread_t *next_wakeup = KCPUS[CPU].wakeups->next;
        thread_t *thread = KCPUS[CPU].wakeups;
        scheduler_add_thread(thread);
        KCPUS[CPU].wakeups = next_wakeup;
        return ready_thread(thread, true);
    }

    thread_t *thread = scheduler_next_thread(KCPUS[CPU].cur);
    if (thread)
        return ready_thread(thread, true);
    
    return ready_thread(KSMP.idle, false);
}

cpu_status_t *ready_thread(thread_t *thread, bool safety) {
    if (safety)
        KCPUS[CPU].cur = thread;
    
    thread->status = READY;
    thread->ticks = 0;
    load_cr3(PADDR(thread->vmm.p4_tbl));
    return thread->ef;
}

thread_t *scheduler_next_thread(thread_t *thread) {
    if (KCPUS[CPU].threads == NULL)
        return NULL;
    
    if (thread == NULL)
        return KCPUS[CPU].threads;
    
    if (thread->next == NULL)
        return KCPUS[CPU].threads;
    
    return thread->next;
}

void scheduler_add_thread(thread_t *thread) {
    size_t cpu = 0;
    size_t threads = KCPUS[0].num_threads;
    for (size_t i = 1; i < KSMP.num_cpus; i++) {
        if (KCPUS[i].num_threads < threads) {
            cpu = i;
            threads = KCPUS[i].num_threads;
        }
    }
    KCPUS[cpu].num_threads++;
    thread->next = KCPUS[cpu].threads;
    KCPUS[cpu].threads = thread;
}

void scheduler_remove_thread(thread_t * volatile *list, thread_t *thread) {
    thread_t *item = *list;
    thread_t *prev_item = NULL;

    while (item != NULL && item != thread) {
        prev_item = item;
        item = item->next;
    }

    if (item == NULL)
        return log(WARNING, "SCHEDULER", "Couldn't find thread");
    
    if (item == *list)
        *list = item->next;
    else
        prev_item->next = item->next;
}
