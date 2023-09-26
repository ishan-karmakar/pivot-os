#pragma once
#include <stddef.h>
#include <cpu/cpu.h>

typedef struct thread_t {
    cpu_status_t *ef;
    struct thread_t *next;
} __attribute__((packed)) thread_t;

extern void create_thread(void (*)(void), uintptr_t);