#include <stddef.h>
#include <mem/pmm.h>
#include <mem/bitmap.h>
#include <io/stdio.h>
#include <kernel/logging.h>
#include <kernel/progress.h>
#include <sys.h>

static bool table_empty(page_table_t);

mem_info_t *mem_info;

void init_pmm(mem_info_t *memory_info) {
    mem_info = memory_info;
    mem_info->pml4 = (page_table_t) VADDR(mem_info->pml4);
    log(Info, "PMM", "PML4 addr: %x", (uintptr_t) mem_info->pml4);
    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", mem_info->mem_pages, mem_info->mem_pages * PAGE_SIZE / 1048576);
    init_bitmap();
    map_higher_half(NULL);
    log(Info, "PMM", "Initialized Physical Memory Manager");
}

void map_higher_half(page_table_t p4_tbl) {
    size_t mmap_num_entries = mem_info->mmap_size / mem_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = mem_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        uint8_t type = current_desc->type;
        if ((type == 2 || type == 3 || type == 4 || type == 7) && current_desc->physical_start != 0)
            map_range(current_desc->physical_start, VADDR(current_desc->physical_start), current_desc->count, p4_tbl);
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    }

    map_range((uintptr_t) mem_info->kernel_entries, VADDR(mem_info->kernel_entries), SIZE_TO_PAGES(mem_info->num_kernel_entries * sizeof(kernel_entry_t)), p4_tbl);
    mem_info->kernel_entries = (kernel_entry_t*) VADDR(mem_info->kernel_entries);
}

void map_threading(page_table_t p4_tbl) {
    size_t mmap_num_entries = mem_info->mmap_size / mem_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = mem_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        uint8_t type = current_desc->type;
        if ((type == 2 || type == 3 || type == 4 || type == 7) && current_desc->physical_start != 0) {
            map_range(current_desc->physical_start, current_desc->physical_start, current_desc->count, p4_tbl);
            map_range(current_desc->physical_start, VADDR(current_desc->physical_start), current_desc->count, p4_tbl);
        }
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    }

    map_range((uintptr_t) mem_info->kernel_entries, VADDR(mem_info->kernel_entries), SIZE_TO_PAGES(mem_info->num_kernel_entries * sizeof(kernel_entry_t)), p4_tbl);
}

/*
State at the end of function:
- All addresses in lower half are unmapped except for page tables
- All physical memory is mapped in the higher half
*/
void cleanup_uefi(void) {
    log(Info, "PMM", "Unmapping lower half...");
    size_t num_pages = 0xFFFFFFFF / 4096;
    create_progress(num_pages);
    for (size_t i = 0; i < num_pages; i++) {
        if (i * PAGE_SIZE != (uintptr_t) mem_info->pml4)
            unmap_addr(i * PAGE_SIZE, NULL);
        update_progress(i + 1);
    }
}

// Maps the kernel entries in a new page table, used for new tasks and threads
void map_kernel_entries(page_table_t p4_tbl) {
    for (size_t i = 0; i < mem_info->num_kernel_entries; i++)
        map_range(mem_info->kernel_entries[i].paddr, mem_info->kernel_entries[i].vaddr, mem_info->kernel_entries[i].num_pages, p4_tbl);
}

void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, page_table_t p4_tbl) {
    for (size_t i = 0; i < num_pages; i++) {
        map_addr(physical + i * PAGE_SIZE, virtual + i * PAGE_SIZE, PAGE_TABLE_ENTRY, p4_tbl);
    }
}

void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, page_table_t p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = mem_info->pml4;

    physical = ALIGN_ADDR(physical);
    virtual = ALIGN_ADDR(virtual);

    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    if (!(p4_tbl[p4_idx] & 1)) {
        page_table_t table = alloc_frame();
        p4_tbl[p4_idx] = (uintptr_t) table | PAGE_TABLE_ENTRY;
        clean_table((page_table_t) VADDR(table));
        map_addr((uintptr_t) table, VADDR(table), PAGE_TABLE_ENTRY, p4_tbl);
    }

    page_table_t p3_tbl = (page_table_t) VADDR(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        page_table_t table = alloc_frame();
        p3_tbl[p3_idx] = (uintptr_t) table | PAGE_TABLE_ENTRY;
        clean_table((page_table_t) VADDR(table));
        map_addr((uintptr_t) table, VADDR(table), PAGE_TABLE_ENTRY, p4_tbl);
    }

    page_table_t p2_tbl = (page_table_t) VADDR(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        page_table_t table = alloc_frame();
        p2_tbl[p2_idx] = (uintptr_t) table | PAGE_TABLE_ENTRY;
        clean_table((page_table_t) VADDR(table));
        map_addr((uintptr_t) table, VADDR(table), PAGE_TABLE_ENTRY, p4_tbl);
    }
    page_table_t p1_tbl = (page_table_t) VADDR(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = physical | flags;
}

void unmap_addr(uintptr_t addr, page_table_t p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = mem_info->pml4;
    
    uint16_t p4_idx = P4_ENTRY(addr);
    uint16_t p3_idx = P3_ENTRY(addr);
    uint16_t p2_idx = P2_ENTRY(addr);
    uint16_t p1_idx = P1_ENTRY(addr);

    if (!(p4_tbl[p4_idx] & 1)) return;
    
    page_table_t p3_tbl = (page_table_t) VADDR(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) return;

    page_table_t p2_tbl = (page_table_t) VADDR(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) return;

    page_table_t p1_tbl = (page_table_t) VADDR(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = 0;
    invlpg(addr);
    return;
    // TODO: Cleanup all tables after unmapping lower half, instead of each iteration
    // This part slows down unmapping a lot, yet to find a better solution though
    if (table_empty(p1_tbl)) {
        bitmap_clear_bit(PADDR((uintptr_t) p1_tbl));
        unmap_addr(VADDR(p1_tbl), p4_tbl);
        p2_tbl[p2_idx] = 0;

        if (table_empty(p2_tbl)) {
            bitmap_clear_bit(PADDR((uintptr_t) p2_tbl));
            unmap_addr(VADDR(p2_tbl), p4_tbl);
            p3_tbl[p3_idx] = 0;
            
            if (table_empty(p3_tbl)) {
                bitmap_clear_bit(PADDR((uintptr_t) p3_tbl));
                unmap_addr(VADDR(p3_tbl), p4_tbl);
                p4_tbl[p4_idx] = 0;
            }
        }
    }
}

uintptr_t get_phys_addr(uintptr_t virtual, page_table_t p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = mem_info->pml4;
    
    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    if (!(p4_tbl[p4_idx] & 1)) return 0;
    
    page_table_t p3_tbl = (page_table_t) VADDR(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) return 0;

    page_table_t p2_tbl = (page_table_t) VADDR(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) return 0;

    page_table_t p1_tbl = (page_table_t) VADDR(p2_tbl[p2_idx] & SIGN_MASK);
    return p1_tbl[p1_idx] & SIGN_MASK;
}

void *alloc_frame(void) {
    int64_t frame = bitmap_request_frame();
    if (frame > 0) {
        bitmap_set_bit(frame * PAGE_SIZE);
        return (void*) (frame * PAGE_SIZE);
    }
    log(Error, "ALLOC_FRAME", "alloc_frame() returned NULL");
    return NULL;
}

void invlpg(uintptr_t addr) {
    asm volatile ("invlpg (%0)" : : "r" (addr) : "memory");
}

void clean_table(page_table_t table) {
    for (int i = 0; i < 512; i++)
        table[i] = 0;
}

static bool table_empty(page_table_t table) {
    for (size_t i = 0; i < 512; i++)
        if (table[i] & 1)
            return false;
    return true;
}

bool addr_in_phys_mem(uintptr_t addr) {
    return addr <= (mem_info->mem_pages * PAGE_SIZE);
}
