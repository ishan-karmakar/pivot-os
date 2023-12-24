#include <stddef.h>
#include <mem/pmm.h>
#include <mem/bitmap.h>
#include <kernel/logging.h>
#include <sys.h>

static uint64_t *p4_tbl;
size_t mem_pages = 0;

static void clean_table(uint64_t *table) {
    for (int i = 0; i < 512; i++)
        table[i] = 0;
}

void init_pmm(boot_info_t *boot_info) {
    p4_tbl = (uint64_t*) VADDR((uintptr_t) boot_info->pml4);
    init_bitmap(boot_info);

    size_t mmap_num_entries = boot_info->mmap_size / boot_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = boot_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        log(Verbose, "MMAP", "[%u] Type: %u - Address: %x - Num Pages: %u", i, current_desc->type, current_desc->physical_start, current_desc->count);
        if (current_desc->type != 7)
            bitmap_rsv_area(current_desc->physical_start, current_desc->count);
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + boot_info->mmap_descriptor_size);
    }
    log(Info, "PMM", "Initialized Physical Memory Manager");
}

void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags) {
    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    if (!(p4_tbl[p4_idx] & 1)) {
        uint64_t *table = alloc_frame();
        clean_table(table);
        p4_tbl[p4_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p3_tbl = (uint64_t*) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        uint64_t *table = alloc_frame();
        clean_table(table);
        p3_tbl[p3_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p2_tbl = (uint64_t*) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        uint64_t *table = alloc_frame();
        clean_table(table);
        p2_tbl[p2_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p1_tbl = (uint64_t*) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = physical | flags;
}

void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        map_addr(physical + i * PAGE_SIZE, virtual + i * PAGE_SIZE, PAGE_TABLE_ENTRY);
}

void map_phys_mem(void) {
    for (size_t i = 0; i < mem_pages; i++)
        map_addr(i * PAGE_SIZE, VADDR(i * PAGE_SIZE), PAGE_TABLE_ENTRY);
    log(Info, "PMM", "Mapped physical memory to higher half");
}
