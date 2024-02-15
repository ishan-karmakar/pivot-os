#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys.h>

#define VMM_RESERVED_SPACE_SIZE 0x14480000000
#define VMM_ITEMS_PER_PAGE (PAGE_SIZE / sizeof(vmm_item_t))

typedef enum vmm_level {
    Supervisor,
    User
} vmm_level_t;

typedef struct vmm_item {
    uintptr_t base;
    size_t size;
    size_t flags;
} vmm_item_t;

typedef struct vmm_container {
    vmm_item_t items[VMM_ITEMS_PER_PAGE];
    struct vmm_container *next;
} vmm_container_t;

typedef struct vmm_info {
    uintptr_t data_start;
    uintptr_t space_start;
    uint64_t *p4_tbl;

    struct {
        size_t cur_index;
        size_t next_addr;
        uint64_t vmm_data_end;

        vmm_container_t *root_container;
        vmm_container_t *cur_container;
    } status;
} vmm_info_t;

void init_vmm(vmm_level_t vmm_level, vmm_info_t*);
void *valloc(size_t size, size_t flags, vmm_info_t*);
void vfree(void *addr);