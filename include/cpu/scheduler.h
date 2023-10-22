#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <cpu/cpu.h>

typedef enum {
    RUNNING,
    STOPPED,
    SLEEPING,
    ENDED
} thread_status_t;

typedef struct thread_t {
    cpu_status_t *ef;
    thread_status_t status;
    struct thread_t *next;
    struct thread_t *prev;
    size_t wakeup_time;
} __attribute__((packed)) thread_t;

void create_thread(void (*)(void), uintptr_t);
void create_failsafe_thread(uintptr_t stack_pointer);
void thread_sleep(size_t);