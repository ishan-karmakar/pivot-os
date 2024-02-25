#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <mem/vmm.h>

#define TASK_NAME_LEN 32
typedef void (*thread_fn_t)(void);

struct thread;

typedef struct task {
    size_t id;
    char name[TASK_NAME_LEN];

    vmm_info_t vmm_info;

    struct thread *threads;
    struct task *next;
} task_t;

void task_add_thread(task_t*, struct thread*);
void task_remove_thread(struct thread*);
task_t *create_task(char *name, thread_fn_t entry_point, bool supervisor, bool add_scheduler_list);