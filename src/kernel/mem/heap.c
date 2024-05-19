#include <sys.h>
#include <mem/heap.h>
#include <mem/vmm.h>
#include <mem/pmm.h>
#include <kernel/logging.h>
#include <libc/string.h>

void heap_add(size_t pages, size_t bsize, vmm_info_t *vmm_info, heap_t *heap) {
    void *addr = valloc(pages, 0, vmm_info);
    heap_region_t *b = (heap_region_t*) addr;
    b->bsize = bsize;
    b->size = (pages * PAGE_SIZE) - sizeof(heap_region_t);
    
    b->next = *heap;
    *heap = b;

    size_t blocks = b->size / b->bsize;
    uint8_t *bm = (uint8_t*) &b[1];

    for (size_t i = 0; i < blocks; i++)
        bm[i] = 0;
    
    blocks = DIV_CEIL(blocks, b->bsize);
    for (size_t i = 0; i < blocks; i++)
        bm[i] = 1;
    
    b->ela = blocks + 1;
    b->used = blocks;

    log(Verbose, "HEAP", "Added a region of %u pages", pages);
}

static uint8_t get_id(uint8_t a, uint8_t b) {
    size_t c;	
	for (c = 1; c == a || c == b; c++);
	return c;
}

static heap_region_t *get_region(void *ptr, heap_t heap) {
    for (heap_region_t *b = heap; b; b = b->next) {
        uintptr_t start = (uintptr_t) &b[1];
        uintptr_t end = start + b->size;
        if (start <= (uintptr_t) ptr && (uintptr_t) ptr < end)
            return b;
    }
    return NULL;
}

// Memory is not guaranteed to be zeroed
void *halloc(size_t size, heap_t heap) {
    for (heap_region_t *b = heap; b; b = b->next) {
        if ((b->size - (b->used * b->bsize)) < size)
            continue;
    
        // The area has enough space, but could be non-contiguous
        size_t nblocks = DIV_CEIL(size, b->bsize);
        size_t tblocks = b->size / b->bsize;

        uint8_t *bm = (uint8_t*) &b[1];
        
        for (size_t i = b->ela; i < tblocks; i++) {
            if (bm[i])
                continue;
            size_t fblocks = 0; // Free blocks
            for (; fblocks < nblocks && (i + fblocks) < tblocks && !bm[i + fblocks]; fblocks++);

            if (fblocks == nblocks) {
                // Have enough contiguous memory
                uint8_t nid = get_id(bm[i - 1], bm[i + fblocks]);

                for (size_t j = 0; j < fblocks; j++)
                    bm[i + j] = nid;
                
                b->ela = i + fblocks; // Don't have to worry about going past limit because it will fail beginning space condition
                b->used += fblocks;
                return (void*) (bm + i * b->bsize);
            }
            i += fblocks;
        }
    }

    return NULL;
}

void hfree(void *ptr, heap_t heap) {
    heap_region_t *b = get_region(ptr, heap);
    if (b) {
        uint8_t *bm = (uint8_t*) &b[1];
        size_t sblock = ((uintptr_t) ptr - (uintptr_t) bm) / b->bsize;
        uint8_t id = bm[sblock];
        for (size_t i = 0; bm[sblock + i] == id && (sblock + i) < (b->size / b->bsize); i++) {
            bm[sblock + i] = 0;
        }
        if (sblock < b->ela)
            b->ela = sblock;
    } else
        log(Warning, "HEAP", "Pointer was not from any known region");
}

void *hrealloc(void *old, size_t size, heap_t heap) {
    void *new = halloc(size, heap);
    heap_region_t *b = get_region(old, heap);
    if (!b) {
        log(Warning, "HEAP", "Old pointer was not from any known region");
        return NULL;
    }
    uint8_t *bm = (uint8_t*) &b[1];
    size_t sblock = ((uintptr_t) old - (uintptr_t) bm) / b->bsize;
    uint8_t id = bm[sblock];
    size_t blocks = 0;
    for (; bm[sblock + blocks] == id && (sblock + blocks) < (b->size / b->bsize); blocks++);
    size_t cpysize = blocks * b->bsize;
    if (size < cpysize)
        cpysize = size;
    memcpy(new, old, size);
    hfree(old, heap);
    return new;
}

void free_heap(vmm_info_t *vmm_info, heap_t heap) {
    for (heap_region_t *b = heap; b; b = b->next)
        vfree(b, DIV_CEIL(b->size, PAGE_SIZE), vmm_info);
}
