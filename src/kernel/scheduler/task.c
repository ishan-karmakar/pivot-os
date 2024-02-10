#include <scheduler/task.h>
#include <mem/kheap.h>
#include <mem/pmm.h>
#include <libc/string.h>

size_t next_task_id = 0;

task_t *create_task(char *name, thread_fn_t entry_point, bool supervisor) {
    task_t *task = (task_t*) kmalloc(sizeof(task_t));
    strcpy(task->name, name);
    task->id = next_task_id++;
}

void task_add_thread(task_t *task, thread_t *thread) {
    thread->next = task->threads;
    thread->next_sibling = thread;
}

void init_task_vmm(task_t *task) {
    task->vmm_info.p4_tbl = (uint64_t*) alloc_frame();

    init_vmm(User, &task->vmm_info);
}