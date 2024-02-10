#include <stddef.h>
#include <scheduler/thread.h>
#include <scheduler/task.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>

size_t scheduler_ticks = 0;

thread_t *thread_list = NULL;
thread_t *cur_thread = NULL;
thread_t *idle_thread = NULL;
task_t *root_task;

size_t thread_list_size = 0;

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

void scheduler_remove_thread(size_t id) {
    thread_t *item = thread_list;
    thread_t *prev_item = NULL;

    while (item != NULL && item->id != id) {
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
    remove_thread(item);
}

void scheduler_yield(void) {
    asm ("int $0x20");
}