#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <cpu/cpu.h>

typedef void (*thread_fn_t)(void);

typedef enum {
    RUNNING,
    STOPPED,
    ENDED
} thread_status_t;

typedef struct thread_t {
    cpu_status_t *ef;
    thread_status_t status;
    struct thread_t *next;
    struct thread_t *prev;
    size_t wakeup_time;
} __attribute__((packed)) thread_t;

void create_thread(thread_fn_t fn, uintptr_t);
void create_failsafe_thread(void);
void thread_sleep(size_t);