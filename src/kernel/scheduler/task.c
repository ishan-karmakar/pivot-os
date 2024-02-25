#include <scheduler/task.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <cpu/lapic.h>
#include <libc/string.h>

size_t next_task_id = 0;

void init_task_vmm(task_t*);

task_t *create_task(char *name, thread_fn_t entry_point, bool supervisor, bool add_scheduler_list) {
    task_t *task = (task_t*) kmalloc(sizeof(task_t));
    strcpy(task->name, name);
    task->id = next_task_id++;
    init_task_vmm(task);
    // struct thread *thread = NULL;
    // if (supervisor)
        // thread = create_thread(name, entry_point, task, supervisor, add_scheduler_list);
    // task->threads = thread;

    // if (add_scheduler_list)
    //     scheduler_add_task(task);

    return task;
}

void task_add_thread(task_t *task, thread_t *thread) {
    thread->next = task->threads;
    thread->next_sibling = thread;
}

void task_remove_thread(thread_t *thread) {
    task_t *task = thread->parent_task;
    thread_t *cur_thread = task->threads;
    thread_t *prev_thread = cur_thread;

    while (cur_thread != NULL) {
        if (cur_thread == thread) {
            if (cur_thread == task->threads)
                task->threads = cur_thread->next_sibling;
            else
                prev_thread->next_sibling = cur_thread->next_sibling;
        }
        prev_thread = cur_thread;
        cur_thread = cur_thread->next_sibling;
    }
}

void init_task_vmm(task_t *task) {
    task->vmm_info.p4_tbl = (uint64_t*) (uintptr_t) alloc_frame();

    init_vmm(User, &task->vmm_info);
    map_addr((uintptr_t) task->vmm_info.p4_tbl, (uintptr_t) task->vmm_info.p4_tbl, PAGE_TABLE_ENTRY, task->vmm_info.p4_tbl);
    // map_kernel_entries(task->vmm_info.p4_tbl);
}