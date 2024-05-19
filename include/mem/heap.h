#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <mem/pmm.h>
#include <mem/vmm.h>

#define DEFAULT_BS 16

typedef struct heap_block {
    uint32_t size;
    uint32_t used;
    uint32_t bsize;
    uint32_t ela; // Block after ELA (end of last alloc)
    struct heap_block *next;
} heap_region_t;

typedef heap_region_t* heap_t;

void heap_add(size_t pages, size_t bsize, vmm_info_t *vmm_info, heap_t *heap);
void *halloc(size_t size, heap_t);
void hfree(void *ptr, heap_t);
void *hrealloc(void *old, size_t size, heap_t);
void free_heap(vmm_info_t *vmm_info, heap_t);