#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/bitmap.h>
#include <sys.h>
#include <kernel/logging.h>

vmm_info_t vmm_kernel;

void init_vmm(vmm_level_t level, size_t max_pages, vmm_info_t *vi) {
    if (vi == NULL) {
        vi = &vmm_kernel;
        vi->p4_tbl = NULL; // Make sure to initialize kernel VMM before calling ANY valloc
    }

    vi->flags = level == Supervisor ? KERNEL_PT_ENTRY : USER_PT_ENTRY;
    vi->bm = (uint8_t*) (HIGHER_HALF_OFFSET + (mem_info->mem_pages + 1) * PAGE_SIZE);
    vi->max_pages = max_pages;
    vi->used = 0;
    vi->ffa = 0;
    size_t bm_pages = DIV_CEIL(max_pages, PAGE_SIZE);

    if (level == Supervisor)
        vi->start = (uintptr_t) vi->bm + bm_pages * PAGE_SIZE;
    else
        vi->start = PAGE_SIZE; // Padding to make sure *0 isn't valid

    for (size_t i = 0; i < bm_pages; i++)
        map_addr((uintptr_t) alloc_frame(), (uintptr_t) vi->bm + i * PAGE_SIZE, vi->flags, vi->p4_tbl);

    for (size_t i = 0; i < max_pages; i++)
        vi->bm[i] = 0;

    log(Info, "VMM", "Initialized virtual memory manager");
}

static uint8_t get_id(uint8_t a, uint8_t b) {
    size_t c;	
	for (c = 1; c == a || c == b; c++);
	return c;
}

void *valloc(size_t pages, size_t flags, vmm_info_t *vi) {
    if (pages == 0)
        return NULL;
    
    if (vi == NULL)
        vi = &vmm_kernel;
    
    if (pages > (vi->max_pages - vi->used)) {
        log(Warning, "VMM", "Not enough pages to fulfill requirement");
        return NULL;
    }

    for (size_t i = vi->ffa; i < vi->max_pages; i++) {
        if (vi->bm[i])
            continue;
        size_t fpages = 0;
        for (; fpages < pages && (i + fpages) < vi->max_pages && !vi->bm[i + fpages]; fpages++);

        if (fpages == pages) {
            uint8_t nid = get_id(i > 0 ? vi->bm[i - 1] : 0, vi->bm[i + fpages]);

            for (size_t j = 0; j < fpages; j++)
                vi->bm[i + j] = nid;
            
            if (i == vi->ffa)
                vi->ffa = i + fpages;
            vi->used += fpages;
            uintptr_t addr = vi->start + i * PAGE_SIZE;
            for (size_t k = 0; k < pages; k++)
                map_addr((uintptr_t) alloc_frame(), addr + k * PAGE_SIZE, flags | vi->flags, vi->p4_tbl);
            return (void*) addr;
        }
    }

    return NULL;
}

void vfree(void *addr, vmm_info_t *vi) {
    if (vi == NULL)
        vi = &vmm_kernel;
    
    size_t start = ((uintptr_t) addr - vi->start) / PAGE_SIZE;
    uint8_t id = vi->bm[start];

    for (size_t i = 0; vi->bm[i] == id && vi->bm[i]; i++) {
        bitmap_clear_bit(get_phys_addr((uintptr_t) addr + i * PAGE_SIZE, vi->p4_tbl));
        vi->bm[i] = 0;
    }

    if (start < vi->ffa)
        vi->ffa = start;
}
