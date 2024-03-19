#include <stddef.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <cpu/tss.h>
#include <scheduler/thread.h>
#include <scheduler/task.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>
#include <io/stdio.h>

thread_t *scheduler_next_thread(thread_t*);
void scheduler_remove_thread(thread_t*);

size_t scheduler_ticks = 0;

thread_t *thread_list = NULL;
thread_t *cur_thread = NULL;
thread_t *idle_thread = NULL;
task_t *root_task;

size_t thread_list_size = 0;

cpu_status_t *schedule(cpu_status_t *cur_status) {
    if (cur_thread == NULL)
        return idle_thread->ef;

    if (cur_thread->status == SLEEP)
        cur_thread->ticks = SCHEDULER_THREAD_TICKS; // Act like the thread went for full duration

    if (cur_thread->ticks++ < SCHEDULER_THREAD_TICKS) {
        return cur_status;
    }
    if (cur_thread->status != NEW) {
        cur_thread->ef = cur_status;
    }
    
    if (cur_thread->status != SLEEP && cur_thread->status != DEAD) {
        cur_thread->status = READY;
    }
    
    thread_t *thread_to_execute = NULL;
    thread_t *prev_thread = cur_thread;
    thread_t *tmp_thread = scheduler_next_thread(cur_thread);

    // First check if any sleeping threads wakeup now
    while (tmp_thread != cur_thread) {
        if (tmp_thread->status == SLEEP) {
            if (apic_ticks > tmp_thread->wakeup_time) {
                tmp_thread->status = READY;
                thread_to_execute = tmp_thread;
                break;
            }
        } else if (tmp_thread->status == DEAD) {
            task_remove_thread(tmp_thread);
            scheduler_remove_thread(tmp_thread);
        }

        tmp_thread = scheduler_next_thread(tmp_thread);
    }
    
    if (thread_to_execute == NULL) {
        tmp_thread = scheduler_next_thread(cur_thread);
        while (tmp_thread != cur_thread) {
            if (tmp_thread->status == READY) {
                thread_to_execute = tmp_thread;
                break;
            }

            tmp_thread = scheduler_next_thread(tmp_thread);
        }
    }

    if (thread_to_execute == NULL)
        thread_to_execute = cur_thread;

    thread_to_execute->status = RUN;
    thread_to_execute->ticks = 0;
    cur_thread = thread_to_execute;
    task_t *task = cur_thread->parent_task;

    printf("CR3: %x\n", task->vmm_info.p4_tbl);
    load_cr3((uintptr_t) task->vmm_info.p4_tbl);
    // kernel_tss.rsp0 = cur_thread->rsp0;
    return cur_thread->ef;
}

thread_t *scheduler_next_thread(thread_t *thread) {
    if (thread_list_size == 0 || thread_list == NULL)
        return NULL;
    
    if (thread == NULL)
        return thread_list;
    
    if (thread->next == NULL)
        return thread_list;
    
    return thread->next;
}

void scheduler_add_task(task_t *task) {
    task->next = root_task;
    root_task = task;
}

void scheduler_add_thread(thread_t *thread) {
    thread->next = thread_list;
    thread_list_size++;
    thread_list = thread;
    if (cur_thread == NULL)
        cur_thread = thread;
}

void scheduler_remove_thread(thread_t *thread) {
    thread_t *item = thread_list;
    thread_t *prev_item = NULL;

    while (item != NULL && item != thread) {
        prev_item = item;
        item = item->next;
    }

    if (item == NULL)
        return log(Warning, "SCHEDULER", "Couldn't find thread with given id");
    
    if (item == thread_list)
        thread_list = thread_list->next;
    else
        prev_item->next = item->next;
    
    thread_list_size--;
    free_thread(item);
}

void scheduler_yield(void) {
    asm ("int $0x20");
}