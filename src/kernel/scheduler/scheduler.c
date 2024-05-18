#include <stddef.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <cpu/tss.h>
#include <libc/string.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>
#include <io/stdio.h>

thread_t *scheduler_next_thread(thread_t*);
void scheduler_remove_thread(thread_t*);
cpu_status_t *ready_thread(thread_t*, bool);

thread_t *thread_list = NULL;
thread_t *cur_thread = NULL;
thread_t *idle_thread = NULL;

cpu_status_t *schedule(cpu_status_t *cur_status) {
    if (thread_list == NULL)
        return idle_thread->ef; // CR3 will already be set to idle_thread's

    if (cur_thread != NULL) {
        if ((cur_thread->ticks++ < SCHEDULER_THREAD_TICKS && cur_thread->status != NEW) && cur_thread->status != DEAD && cur_thread->status != SLEEP)
            return cur_status;

        if (cur_thread->status != DEAD && cur_thread->status != SLEEP)
            cur_thread->status = READY;

        memcpy(cur_thread->ef, cur_status, sizeof(cpu_status_t));
    }

    thread_t *tmp_thread = thread_list;
    do {
        if (tmp_thread->status == SLEEP && tmp_thread->wakeup_time <= apic_ticks) {
            return ready_thread(tmp_thread, true);
        } else if (tmp_thread->status == DEAD) {
            thread_t *thread_to_delete = tmp_thread;
            tmp_thread = scheduler_next_thread(tmp_thread);
            scheduler_remove_thread(thread_to_delete);
            free_thread(thread_to_delete);
            if (thread_to_delete == cur_thread) {
                cur_thread = NULL;
                if (thread_list == NULL)
                    return ready_thread(idle_thread, false);
            }
            continue;
        }

        tmp_thread = scheduler_next_thread(tmp_thread);
    } while (tmp_thread != thread_list);

    tmp_thread = thread_list;
    do {
        if (tmp_thread != cur_thread)
            if (tmp_thread->status == READY || tmp_thread->status == NEW)
                return ready_thread(tmp_thread, true);
        tmp_thread = scheduler_next_thread(tmp_thread);
    } while (tmp_thread != thread_list);
    if (cur_thread && (cur_thread->status == READY || cur_thread->status == NEW))
        return ready_thread(cur_thread, true);
    
    return ready_thread(idle_thread, false);
}

cpu_status_t *ready_thread(thread_t *thread, bool safety) {
    if (safety) {
        cur_thread = thread;
        thread->status = READY;
        thread->ticks = 0;
    }

    load_cr3(PADDR(thread->vmm_info.p4_tbl));
    return thread->ef;
}

thread_t *scheduler_next_thread(thread_t *thread) {
    if (thread_list == NULL)
        return NULL;
    
    if (thread == NULL)
        return thread_list;
    
    if (thread->next == NULL)
        return thread_list;
    
    return thread->next;
}

void scheduler_add_thread(thread_t *thread) {
    thread->next = thread_list;
    thread_list = thread;
}

void scheduler_remove_thread(thread_t *thread) {
    thread_t *item = thread_list;
    thread_t *prev_item = NULL;

    while (item != NULL && item != thread) {
        prev_item = item;
        item = item->next;
    }

    if (item == NULL)
        return log(Warning, "SCHEDULER", "Couldn't find thread");
    
    if (item == thread_list)
        thread_list = thread_list->next;
    else
        prev_item->next = item->next;
}

void scheduler_yield(void) {
    asm ("int $0x20");
}