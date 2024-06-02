#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <cpu/cpu.h>
#include <mem/vmm.h>
#include <mem/heap.h>

#define THREAD_NAME_MAX_LEN 32
#define THREAD_MAX_ID (uint16_t - 1)

#define THREAD_STACK_PAGES 1

typedef void (*thread_fn_t)(void);

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
    uintptr_t stack;
    thread_status_t status;
    size_t ticks;
    cpu_status_t *ef;
    size_t wakeup_time;
    vmm_t vmm_info;
    heap_t *heap;
    heap_t *kheap;
    struct thread *next;
} thread_t;

thread_t *create_thread(char *name, thread_fn_t entry_point, bool safety, heap_t *hi);
void free_thread(thread_t*);
void idle(void);
void thread_sleep(size_t);
void thread_yield(void);
void thread_sleep_syscall(cpu_status_t*);
void thread_dead_syscall(void);