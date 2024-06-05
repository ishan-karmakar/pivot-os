#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/bitmap.h>

#define HEAP_DEFAULT_BS 16

typedef struct heap_block {
    bitmap_t bm;
    struct heap_block *next;
} heap_t;

heap_t *heap_add(size_t pages, size_t bsize, vmm_t *vi, heap_t*);
void *halloc(size_t size, heap_t*);
void hfree(void *ptr, heap_t*);
void *hrealloc(void *old, size_t size, heap_t);
void free_heap(vmm_t *vmm_info, heap_t*);
void copy_heap(heap_t*, page_table_t, page_table_t);