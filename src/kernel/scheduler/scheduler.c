#include <stddef.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <cpu/tss.h>
#include <libc/string.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>
#include <io/stdio.h>
#include <kernel.h>

thread_t *scheduler_next_thread(thread_t*);
void scheduler_remove_thread(thread_t**, thread_t*);
cpu_status_t *ready_thread(thread_t*, bool);

cpu_status_t *schedule(cpu_status_t *cur_status) {
    if (!KSCHED.threads && !KSCHED.wakeups)
        return KSCHED.idle->ef; // CR3 will already be set to idle_thread's

    if (KSCHED.cur != NULL) {
        if ((KSCHED.wakeups && KSCHED.wakeups->wakeup_time > KLAPIC.apic_ticks) &&
            KSCHED.cur->ticks++ < SCHEDULER_THREAD_TICKS &&
            KSCHED.cur->status != DEAD && KSCHED.cur->status != SLEEP)
            return cur_status;

        if (KSCHED.cur->status != DEAD && KSCHED.cur->status != SLEEP)
            KSCHED.cur->status = READY;

        memcpy(KSCHED.cur->ef, cur_status, sizeof(cpu_status_t));

        if (KSCHED.cur->status == DEAD) {
            scheduler_remove_thread(&KSCHED.threads, KSCHED.cur);
            KSCHED.cur = NULL;
        } else if (KSCHED.cur->status == SLEEP) {
            thread_t *thread = KSCHED.cur;
            KSCHED.cur = NULL;
            scheduler_remove_thread(&KSCHED.threads, thread);
            thread_t *prev = NULL;
            thread_t *item = KSCHED.wakeups;
            while (item != NULL && item->wakeup_time < thread->wakeup_time) {
                prev = item;
                item = item->next;
            }
            if (prev)
                prev->next = thread;
            else
                KSCHED.wakeups = thread;
            thread->next = item;
        }
    }

    if (KSCHED.wakeups && KSCHED.wakeups->wakeup_time <= KLAPIC.apic_ticks) {
        thread_t *next_wakeup = KSCHED.wakeups->next;
        thread_t *thread = KSCHED.wakeups;
        scheduler_add_thread(thread);
        KSCHED.wakeups = next_wakeup;
        return ready_thread(thread, true);
    }

    // thread_t *tmp_thread = KSCHED.threads;
    // do {
    //     if (tmp_thread->status == SLEEP && tmp_thread->wakeup_time <= KLAPIC.apic_ticks) {
    //         return ready_thread(tmp_thread, true);
    //     } else if (tmp_thread->status == DEAD) {
    //         thread_t *thread_to_delete = tmp_thread;
    //         tmp_thread = scheduler_next_thread(tmp_thread);
    //         scheduler_remove_thread(thread_to_delete);
    //         // free_thread(thread_to_delete);
    //         if (thread_to_delete == KSCHED.cur) {
    //             KSCHED.cur = NULL;
    //             if (KSCHED.threads == NULL)
    //                 return ready_thread(KSCHED.idle, false);
    //         }
    //         continue;
    //     }

    //     tmp_thread = scheduler_next_thread(tmp_thread);
    // } while (tmp_thread != KSCHED.threads);

    thread_t *thread = scheduler_next_thread(KSCHED.cur);
    if (thread)
        return ready_thread(thread, true);
    
    return ready_thread(KSCHED.idle, false);
}

cpu_status_t *ready_thread(thread_t *thread, bool safety) {
    if (safety) {
        KSCHED.cur = thread;
        thread->status = READY;
        thread->ticks = 0;
    }

    load_cr3(PADDR(thread->vmm_info.p4_tbl));
    return thread->ef;
}

thread_t *scheduler_next_thread(thread_t *thread) {
    if (KSCHED.threads == NULL)
        return NULL;
    
    if (thread == NULL)
        return KSCHED.threads;
    
    if (thread->next == NULL)
        return KSCHED.threads;
    
    return thread->next;
}

void scheduler_add_thread(thread_t *thread) {
    thread->next = KSCHED.threads;
    KSCHED.threads = thread;
}

void scheduler_remove_thread(thread_t **list, thread_t *thread) {
    thread_t *item = *list;
    thread_t *prev_item = NULL;

    while (item != NULL && item != thread) {
        prev_item = item;
        item = item->next;
    }

    if (item == NULL)
        return log(Warning, "SCHEDULER", "Couldn't find thread");
    
    if (item == *list)
        *list = item->next;
    else
        prev_item->next = item->next;
}
