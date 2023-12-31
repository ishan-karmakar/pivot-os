#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <cpu/cpu.h>

typedef void (*thread_fn_t)(void);

typedef struct thread_t {
    cpu_status_t *ef;
    bool ended;
    struct thread_t *next;
    struct thread_t *prev;
    size_t wakeup_time;
} __attribute__((packed)) thread_t;

void create_thread(thread_fn_t fn, uintptr_t);
void create_failsafe_thread(void);
void thread_sleep(size_t);
void init_scheduler(thread_fn_t);
void start_scheduler(void);
void print_threads(void);