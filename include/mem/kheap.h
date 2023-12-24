#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define HEAP_ALLOC_ALIGNMENT 0x10
#define KHEAP_MINIMUM_ALLOCABLE_SIZE 0x20

#define MERGE_RIGHT 0b01
#define MERGE_LEFT  0b10
#define MERGE_BOTH  0b11
#define MERGE_NONE  0b00

typedef struct kheap_mem_node_t {
    uint64_t size;
    bool is_free;
    struct kheap_mem_node_t *next;
    struct kheap_mem_node_t *prev;
} kheap_mem_node_t;

void init_kheap(void);
void *kmalloc(size_t);
void kfree(void*);
void *krealloc(void*, size_t);