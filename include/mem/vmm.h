#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys.h>

#define VMM_RESERVED_SPACE_SIZE (20 * PAGE_SIZE)
#define VMM_ITEMS_PER_PAGE (PAGE_SIZE / sizeof(vmm_item_t))

typedef enum vmm_level {
    Supervisor,
    User
} vmm_level_t;

typedef struct vmm_info {
    size_t flags;
    uintptr_t *p4_tbl;
    size_t max_pages;
    uintptr_t start;
    size_t used;
    size_t ffa;
    uint8_t *bm;
} vmm_info_t;

void init_vmm(vmm_level_t vmm_level, size_t max_pages, vmm_info_t*);
void *valloc(size_t pages, size_t flags, vmm_info_t*);
void vfree(void *addr, size_t pages, vmm_info_t*);