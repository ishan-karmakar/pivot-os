#include <mem/heap.h>
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <util/logger.h>
#include <libc/string.h>
#include <kernel.h>
#include <common.h>

heap_t *heap_add(size_t pages, size_t bsize, vmm_t *vi, heap_t *n) {
    heap_t *h = valloc(pages, vi);
    h->bm.bm = (uint8_t*) h + sizeof(heap_t);
    init_bitmap(&h->bm, pages * PAGE_SIZE - sizeof(heap_t), bsize);
    h->next = n;

    return h;
}

// Memory is not guaranteed to be zeroed
void *halloc(size_t size, heap_t *heap) {
    for (heap_t *b = heap; b; b = b->next) {
        void *addr = bm_alloc(size, &b->bm);
        if (addr)
            return addr;
    }

    return NULL;
}

void hfree(void *ptr, heap_t *heap) {
    for (heap_t *b = heap; b; b = b->next)
        if (bm_free(ptr, &heap->bm))
            return;
    log(Warning, "HEAP", "Pointer was not from any known region");
}

void *hrealloc(void *old, size_t size, heap_t *heap) {
    for (heap_t *b = heap; b; b = b->next) {
        void *a = bm_realloc(old, size, &heap->bm);
        if (a)
            return a;
    }
    log(Warning, "HEAP", "Pointer was not from any known region");
    return NULL;
}

void map_heap(heap_t *src, page_table_t src_vmm, page_table_t dest) {
    for (heap_t *b = src; b; b = b->next) {
        size_t num_pages = DIV_CEIL(b->bm.size + sizeof(heap_t), PAGE_SIZE);
        uintptr_t start = (uintptr_t) b->bm.bm - sizeof(heap_t);
        for (size_t i = 0; i < num_pages; i++)
            map_addr(translate_addr(start + i * PAGE_SIZE, src_vmm), start + i * PAGE_SIZE, KERNEL_PT_ENTRY, dest);
    }
}

void free_heap(vmm_t *vmm, heap_t *heap) {
    for (heap_t *b = heap; b; b = b->next)
        vfree(b, vmm);
}
