#include <mem/vmm.h>
#include <mem/pmm.h>
#include <sys.h>
#include <kernel/logging.h>

vmm_t vmm_kernel;

void init_vmm(vmm_level_t level, size_t max_pages, vmm_t *vi) {
    if (!vi) {
        vi = &vmm_kernel;
        vi->p4_tbl = NULL;
    }

    size_t flags = level == Supervisor ? KERNEL_PT_ENTRY : USER_PT_ENTRY;
    vi->flags = flags;
    if (level == Supervisor)
        vi->bm.bm = (uint8_t*) (HIGHER_HALF_OFFSET + mem_info->mem_pages * PAGE_SIZE);
    else
        vi->bm.bm = (uint8_t*) PAGE_SIZE;
    
    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) vi->bm.bm + i * PAGE_SIZE, flags, vi->p4_tbl);

    init_bitmap(&vi->bm, max_pages * PAGE_SIZE, PAGE_SIZE);
    log(Info, "VMM", "Initialized virtual memory manager");
}

void *valloc(size_t pages, vmm_t *vi) {
    if (pages == 0)
        return NULL;
    
    if (!vi)
        vi = &vmm_kernel;

    void *addr = bm_alloc(pages * PAGE_SIZE, &vi->bm);
    for (size_t i = 0; i < pages; i++)
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) addr + i * PAGE_SIZE, vi->flags, vi->p4_tbl);
    
    return addr;
}

void vfree(void *addr, vmm_t *vi) {
    if (vi == NULL)
        vi = &vmm_kernel;
    
    size_t pages = bm_free(addr, &vi->bm) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++)
        pmm_clear_bit(get_phys_addr((uintptr_t) addr + i * PAGE_SIZE, vi->p4_tbl));
}
