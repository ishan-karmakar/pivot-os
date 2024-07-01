#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mem/bitmap.h>
#include <mem/pmm.h>

#define VMM_RESERVED_SPACE_SIZE (20 * PAGE_SIZE)
#define VMM_ITEMS_PER_PAGE (PAGE_SIZE / sizeof(vmm_item_t))

typedef enum vmm_level {
    Supervisor,
    User
} vmm_level_t;

typedef struct vmm {
    size_t flags;
    page_table_t p4_tbl;
    bitmap_t bm;
} vmm_t;

void init_vmm(vmm_level_t vmm_level, size_t max_pages, vmm_t*);
void *valloc(size_t pages, vmm_t*);
void vfree(void *addr, vmm_t*);