#include <mem/vmm.h>
#include <mem/pmm.h>
#include <util/logger.h>
#include <kernel.h>
#include <common.h>

void init_vmm(vmm_level_t level, size_t max_pages, vmm_t *vi) {
    vi->flags = level == Supervisor ? KERNEL_PT_ENTRY : USER_PT_ENTRY;
    if (level == Supervisor)
        vi->bm.bm = (uint8_t*) HIGHER_HALF_OFFSET;
    else
        vi->bm.bm = (uint8_t*) PAGE_SIZE;
    
    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++) {
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) vi->bm.bm + i * PAGE_SIZE, vi->flags, vi->p4_tbl);
    }
    init_bitmap(&vi->bm, max_pages * PAGE_SIZE, PAGE_SIZE);
    log(Info, "VMM", "Initialized virtual memory manager");
}

void *valloc(size_t pages, vmm_t *vi) {
    if (pages == 0)
        return NULL;
    
    void *addr = bm_alloc(pages * PAGE_SIZE, &vi->bm);
    for (size_t i = 0; i < pages; i++)
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) addr + i * PAGE_SIZE, vi->flags, vi->p4_tbl);
    
    return addr;
}

void vfree(void *addr, vmm_t *vi) {
    size_t pages = bm_free(addr, &vi->bm) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++)
        pmm_clear_bit(translate_addr((uintptr_t) addr + i * PAGE_SIZE, vi->p4_tbl));
}
