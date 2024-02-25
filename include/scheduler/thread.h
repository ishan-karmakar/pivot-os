#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cpu/cpu.h>
#include <scheduler/task.h>

#define THREAD_NAME_MAX_LEN 32
#define THREAD_MAX_ID (uint16_t - 1)

#define THREAD_DEFAULT_STACK_SIZE 0x10000

typedef enum thread_status {
    NEW,
    INIT,
    RUN,
    READY,
    SLEEP,
    WAIT,
    DEAD
} thread_status_t;

struct task;

typedef struct thread {
    uint16_t id;
    char name[THREAD_NAME_MAX_LEN];
    uintptr_t rsp0;
    uintptr_t stack;
    struct task *parent_task;
    thread_status_t status;
    size_t ticks;
    cpu_status_t *ef;
    size_t wakeup_time;
    struct thread *next_sibling;
    struct thread *next;
} thread_t;

thread_t *create_thread(char *name, thread_fn_t entry_point, struct task *parent_task, bool supervisor, bool add_scheduler_list);
void free_thread(thread_t*);
void idle(void);