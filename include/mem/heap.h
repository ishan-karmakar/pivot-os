#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#define HEAP_ALLOC_ALIGNMENT 0x10
#define HEAP_MINIMUM_ALLOCABLE_SIZE 0x20

#define MERGE_RIGHT 0b01
#define MERGE_LEFT  0b10
#define MERGE_BOTH  0b11
#define MERGE_NONE  0b00

typedef struct heap_mem_node {
    uint64_t size;
    bool is_free;
    struct heap_mem_node *next;
    struct heap_mem_node *prev;
} heap_mem_node_t;

typedef struct {
    heap_mem_node_t *heap_start;
    heap_mem_node_t *heap_end;
} heap_info_t;

void init_heap(heap_info_t*, vmm_info_t*);
void set_heap(heap_info_t*);
void *malloc(size_t);
void free(void*);
void *realloc(void*, size_t);