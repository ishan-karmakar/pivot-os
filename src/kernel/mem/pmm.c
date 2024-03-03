#include <stddef.h>
#include <mem/pmm.h>
#include <mem/bitmap.h>
#include <kernel/logging.h>
#include <sys.h>

static bool table_empty(uint64_t *table);

mem_info_t *mem_info;

void init_pmm(mem_info_t *memory_info) {
    mem_info = memory_info;
    mem_info->pml4 = (uint64_t*) VADDR((uintptr_t) mem_info->pml4);
    mem_info->bitmap = (uint64_t*) VADDR((uintptr_t) mem_info->bitmap);
    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", mem_info->mem_pages, mem_info->mem_pages * PAGE_SIZE / 1048576);
    init_bitmap(mem_info);

    size_t mmap_num_entries = mem_info->mmap_size / mem_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = mem_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        log(Debug, "MMAP", "[%u] Type: %u - Address: %x - Num Pages: %u", i, current_desc->type, current_desc->physical_start, current_desc->count);
        if (current_desc->type != 7)
            bitmap_rsv_area(current_desc->physical_start, current_desc->count);
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    }
    
    for (size_t i = 0; i < mem_info->num_kernel_entries; i++) {
        log(Debug, "KERNEL_ENTRY", "[%u] (Physical) %x -> (Virtual) %x - %u pages", i, mem_info->kernel_entries[i].paddr, mem_info->kernel_entries[i].vaddr, mem_info->kernel_entries[i].num_pages);
        bitmap_rsv_area(mem_info->kernel_entries[i].paddr, mem_info->kernel_entries[i].num_pages);
    }

    log(Info, "PMM", "Initialized Physical Memory Manager");

    for (size_t i = 0; i < (0xFFFFFFFF / 4096); i++)
        unmap_addr(i * PAGE_SIZE, NULL);
    log(Verbose, "PMM", "Unmapped lower half");

    for (size_t i = 0; i < mem_info->mem_pages; i++)
        map_addr(i * PAGE_SIZE, VADDR(i * PAGE_SIZE), PAGE_TABLE_ENTRY, NULL);
    log(Verbose, "PMM", "Mapped higher half");
}

static void clean_table(uint64_t *table) {
    for (int i = 0; i < 512; i++)
        table[i] = 0;
}

void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, uint64_t *p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = mem_info->pml4;

    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    log(Debug, "PMM", "Mapping %x (V) to %x (P) (4: %u, 3: %u, 2: %u, 1: %u)", virtual, physical, p4_idx, p3_idx, p2_idx, p1_idx);

    if (!(p4_tbl[p4_idx] & 1)) {
        uint64_t *table = alloc_frame();
        map_addr((uintptr_t) table, (uintptr_t) table, PAGE_TABLE_ENTRY, p4_tbl);
        clean_table(table);
        p4_tbl[p4_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p3_tbl = (uint64_t*) VADDR(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        uint64_t *table = alloc_frame();
        map_addr((uintptr_t) table, (uintptr_t) table, PAGE_TABLE_ENTRY, p4_tbl);
        clean_table(table);
        p3_tbl[p3_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p2_tbl = (uint64_t*) VADDR(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        uint64_t *table = alloc_frame();
        map_addr((uintptr_t) table, (uintptr_t) table, PAGE_TABLE_ENTRY, p4_tbl);
        clean_table(table);
        p2_tbl[p2_idx] = (uintptr_t) table | flags;
    }

    uint64_t *p1_tbl = (uint64_t*) VADDR(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = physical | flags;
}

void unmap_addr(uintptr_t addr, uint64_t *p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = mem_info->pml4;
    
    uint16_t p4_idx = P4_ENTRY(addr);
    uint16_t p3_idx = P3_ENTRY(addr);
    uint16_t p2_idx = P2_ENTRY(addr);
    uint16_t p1_idx = P1_ENTRY(addr);

    if (!(p4_tbl[p4_idx] & 1)) return;
    
    uint64_t *p3_tbl = (uint64_t*) VADDR(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) return;

    uint64_t *p2_tbl = (uint64_t*) VADDR(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) return;

    uint64_t *p1_tbl = (uint64_t*) VADDR(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = 0;

    if (table_empty(p1_tbl)) {
        bitmap_clear_bit(PADDR((uintptr_t) p1_tbl));
        p2_tbl[p2_idx] = 0;

        if (table_empty(p2_tbl)) {
            bitmap_clear_bit(PADDR((uintptr_t) p2_tbl));
            p3_tbl[p3_idx] = 0;
            
            if (table_empty(p3_tbl)) {
                bitmap_clear_bit(PADDR((uintptr_t) p3_tbl));
                p4_tbl[p4_idx] = 0;
            }
        }
    }
}

static bool table_empty(uint64_t *table) {
    for (size_t i = 0; i < 512; i++)
        if (table[i] & 1)
            return false;
    return true;
}

void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, uint64_t *p4_tbl) {
    for (size_t i = 0; i < num_pages; i++)
        map_addr(physical + i * PAGE_SIZE, virtual + i * PAGE_SIZE, PAGE_TABLE_ENTRY, p4_tbl);
}

// Maps the kernel entries in a new page table, used for new tasks and threads
void map_kernel_entries(uint64_t *p4_tbl) {
    for (size_t i = 0; i < mem_info->num_kernel_entries; i++)
        map_addr(mem_info->kernel_entries[i].paddr, mem_info->kernel_entries[i].vaddr, PAGE_TABLE_ENTRY, p4_tbl);
}

void *alloc_frame(void) {
    int64_t frame = bitmap_request_frame();
    if (frame > 0) {
        bitmap_set_bit(frame * PAGE_SIZE);
        return (void*) (frame * PAGE_SIZE);
    }

    return NULL;
}

bool addr_in_phys_mem(uintptr_t addr) {
    return addr <= (mem_info->mem_pages * PAGE_SIZE);
}
