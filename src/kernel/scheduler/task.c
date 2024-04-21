#include <scheduler/task.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <kernel/logging.h>
#include <cpu/lapic.h>
#include <libc/string.h>
#include <drivers/framebuffer.h>

size_t next_task_id = 0;

void init_task_vmm(task_t*);

task_t *create_task(char *name, thread_fn_t entry_point, bool supervisor, bool add_scheduler_list) {
    task_t *task = (task_t*) kmalloc(sizeof(task_t));
    strcpy(task->name, name);
    task->id = next_task_id++;
    init_task_vmm(task);
    struct thread *thread = NULL;
    if (supervisor)
        thread = create_thread(name, entry_point, task, supervisor, add_scheduler_list);
    task->threads = thread;

    if (add_scheduler_list)
        scheduler_add_task(task);

    log(Info, "TASK", "Created %s task", name);

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

/*
- Kernel entries
- Identity map PML4
- Map first 2MB
*/
void init_task_vmm(task_t *task) {
    task->vmm_info.p4_tbl = (page_table_t) VADDR(alloc_frame());
    log(Verbose, "TASK", "%x", (uintptr_t) task->vmm_info.p4_tbl);
    clean_table(task->vmm_info.p4_tbl);
    map_addr(PADDR(task->vmm_info.p4_tbl), (uintptr_t) task->vmm_info.p4_tbl, PAGE_TABLE_ENTRY, task->vmm_info.p4_tbl);
    map_addr(PADDR(task->vmm_info.p4_tbl), PADDR(task->vmm_info.p4_tbl), PAGE_TABLE_ENTRY, task->vmm_info.p4_tbl);

    init_vmm(User, &task->vmm_info);
    map_kernel_entries(task->vmm_info.p4_tbl);
    map_framebuffer(task->vmm_info.p4_tbl);
    map_lapic(task->vmm_info.p4_tbl);
    map_kheap(task->vmm_info.p4_tbl);
    map_threading(task->vmm_info.p4_tbl);

    // valloc(PAGE_SIZE * 512, 0, &task->vmm_info);
}