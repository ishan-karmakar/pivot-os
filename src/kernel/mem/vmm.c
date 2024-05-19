#include <mem/vmm.h>
#include <mem/pmm.h>
#include <sys.h>
#include <kernel/logging.h>

vmm_info_t vmm_kernel;

void init_vmm(vmm_level_t level, size_t max_pages, vmm_info_t *vmm_info) {
    if (vmm_info == NULL) {
        vmm_info = &vmm_kernel;
        vmm_info->p4_tbl = NULL; // Make sure to initialize kernel VMM before calling ANY valloc
    }

    vmm_info->flags = level == Supervisor ? KERNEL_PT_ENTRY : USER_PT_ENTRY;
    vmm_info->bm = (uint8_t*) (HIGHER_HALF_OFFSET + (mem_info->mem_pages + 1) * PAGE_SIZE);
    vmm_info->max_pages = max_pages;
    vmm_info->used = 0;
    vmm_info->ffa = 0;
    size_t bm_pages = DIV_CEIL(max_pages, PAGE_SIZE);

    if (level == Supervisor)
        vmm_info->start = (uintptr_t) vmm_info->bm + bm_pages * PAGE_SIZE;
    else
        vmm_info->start = PAGE_SIZE; // Padding to make sure *0 isn't valid

    for (size_t i = 0; i < bm_pages; i++)
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) vmm_info->bm + i * PAGE_SIZE, vmm_info->flags, vmm_info->p4_tbl);

    for (size_t i = 0; i < max_pages; i++)
        vmm_info->bm[i] = 0;

    log(Info, "VMM", "Initialized virtual memory manager");
}

static uint8_t get_id(uint8_t a, uint8_t b) {
    size_t c;	
	for (c = 1; c == a || c == b; c++);
	return c;
}

void *valloc(size_t pages, size_t flags, vmm_info_t *vmm_info) {
    if (pages == 0)
        return NULL;
    
    if (vmm_info == NULL)
        vmm_info = &vmm_kernel;
    
    if (pages > (vmm_info->max_pages - vmm_info->used)) {
        log(Warning, "VMM", "Not enough pages to fulfill requirement");
        return NULL;
    }

    for (size_t i = vmm_info->ffa; i < vmm_info->max_pages; i++) {
        if (vmm_info->bm[i])
            continue;
        size_t fpages = 0;
        for (; fpages < pages && (i + fpages) < vmm_info->max_pages && !vmm_info->bm[i + fpages]; fpages++);

        if (fpages == pages) {
            log(Verbose, "VMM", "");
            uint8_t nid = get_id(i > 0 ? vmm_info->bm[i - 1] : 0, vmm_info->bm[i + fpages]);

            for (size_t j = 0; j < fpages; j++)
                vmm_info->bm[i + j] = nid;
            
            vmm_info->ffa = i + fpages;
            vmm_info->used += fpages;
            uintptr_t addr = vmm_info->start + i * PAGE_SIZE;
            for (size_t k = 0; k < pages; k++)
                map_addr((uintptr_t) alloc_frame(), addr + k * PAGE_SIZE, vmm_info->flags, vmm_info->p4_tbl);
            return (void*) addr;
        }
    }

    return NULL;
}

void vfree(void *addr, size_t pages, vmm_info_t *vmm_info) {
    
}
