/*
NOTE:
The PMM uses a bitmap, but it actually uses the individual bits to store used pages.
As a result, the PMM doesn't use the bitmap from bitmap.h.
Only the VMM and HEAP use the bitmap from bitmap.h
*/

#include <stddef.h>
#include <mem/pmm.h>
#include <io/stdio.h>
#include <kernel/logging.h>
#include <kernel/progress.h>
#include <kernel.h>

static bool table_empty(page_table_t);
static int64_t pmm_request_frame();

void init_pmm(void) {
    KVMM.p4_tbl = KMEM.pml4;
    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", KMEM.mem_pages, KMEM.mem_pages * PAGE_SIZE / 1048576);
    for (size_t i = 0; i < KMEM.bitmap_entries; i++)
        KMEM.bitmap[i] = BITMAP_ROW_FULL;
    log(Verbose, "PMM", "Initialized bitmap");
    pmm_set_bit(0x8000); // Need for SMP
    size_t mmap_num_entries = KMEM.mmap_size / KMEM.mmap_desc_size;
    mmap_desc_t *current_desc = KMEM.mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        log(Debug, "MMAP", "[%u] Type: %u - Address: %x - Num Pages: %u", i, current_desc->type, current_desc->physical_start, current_desc->count);
        uint8_t type = current_desc->type;
        if (type == 3 || type == 4 || type == 7)
            pmm_clear_area(current_desc->physical_start, current_desc->count);
        current_desc = (mmap_desc_t*) ((uint8_t*) current_desc + KMEM.mmap_desc_size);
    }
    log(Info, "PMM", "Initialized Physical Memory Manager");
}

// Maps the kernel entries in a new page table, used for new threads
void map_kernel_entries(page_table_t p4_tbl) {
    for (size_t i = 0; i < KMEM.num_ke; i++) {
        log(Verbose, "PMM", "[%u] %x, %x, %u", i, KMEM.ke[i].paddr, KMEM.ke[i].vaddr, KMEM.ke[i].num_pages);
        // TODO: Once all the functions the user process calls are bundled together, map_range should be KERNEL_PT_ENTRY
        // Otherwise, user function could corrupt kernel code
        map_range(KMEM.ke[i].paddr, KMEM.ke[i].vaddr, KMEM.ke[i].num_pages, USER_PT_ENTRY, p4_tbl);
    }
}

void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, size_t flags, page_table_t p4_tbl) {
    for (size_t i = 0; i < num_pages; i++) {
        map_addr(physical + i * PAGE_SIZE, virtual + i * PAGE_SIZE, flags, p4_tbl);
    }
}

void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, page_table_t p4_tbl) {
    if (physical == 0 || virtual == 0)
        return;
    physical = ALIGN_ADDR(physical);
    virtual = ALIGN_ADDR(virtual);

    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    if (!(p4_tbl[p4_idx] & 1)) {
        page_table_t table = alloc_frame();
        p4_tbl[p4_idx] = (uintptr_t) table | USER_PT_ENTRY;
        clean_table((page_table_t) table);
        map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
    }

    page_table_t p3_tbl = (page_table_t) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        page_table_t table = alloc_frame();
        p3_tbl[p3_idx] = (uintptr_t) table | USER_PT_ENTRY;
        clean_table((page_table_t) table);
        map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
    }

    page_table_t p2_tbl = (page_table_t) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        page_table_t table = alloc_frame();
        p2_tbl[p2_idx] = (uintptr_t) table | USER_PT_ENTRY;
        clean_table((page_table_t) table);
        map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
    }

    page_table_t p1_tbl = (page_table_t) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = physical | flags;
}

void unmap_addr(uintptr_t addr, page_table_t p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = KMEM.pml4;
    
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
        pmm_clear_bit(PADDR((uintptr_t) p1_tbl));
        unmap_addr(VADDR(p1_tbl), p4_tbl);
        p2_tbl[p2_idx] = 0;

        if (table_empty(p2_tbl)) {
            pmm_clear_bit(PADDR((uintptr_t) p2_tbl));
            unmap_addr(VADDR(p2_tbl), p4_tbl);
            p3_tbl[p3_idx] = 0;
            
            if (table_empty(p3_tbl)) {
                pmm_clear_bit(PADDR((uintptr_t) p3_tbl));
                unmap_addr(VADDR(p3_tbl), p4_tbl);
                p4_tbl[p4_idx] = 0;
            }
        }
    }
}

uintptr_t translate_addr(uintptr_t virtual, page_table_t p4_tbl) {
    if (p4_tbl == NULL)
        p4_tbl = KMEM.pml4;
    
    virtual = ALIGN_ADDR(virtual);
    uint16_t p4_idx = P4_ENTRY(virtual);
    uint16_t p3_idx = P3_ENTRY(virtual);
    uint16_t p2_idx = P2_ENTRY(virtual);
    uint16_t p1_idx = P1_ENTRY(virtual);

    if (!(p4_tbl[p4_idx] & 1)) return 0;
    
    page_table_t p3_tbl = (page_table_t) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) return 0;

    page_table_t p2_tbl = (page_table_t) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) return 0;

    page_table_t p1_tbl = (page_table_t) (p2_tbl[p2_idx] & SIGN_MASK);
    return p1_tbl[p1_idx] & SIGN_MASK;
}

void free_page_table(page_table_t table, uint8_t level) {
    if (level > 1) {
        for (size_t i = 0; i < 512; i++)
            if (table[i] & 1)
                free_page_table((page_table_t) (table[i] & SIGN_MASK), level - 1);
    }

    pmm_clear_bit(PADDR(table));
}

void map_pmm(page_table_t table) {
    size_t num_pages = DIV_CEIL(KMEM.bitmap_size, PAGE_SIZE);
    for (size_t i = 0; i < num_pages; i++)
        map_addr((uintptr_t) KMEM.bitmap + i * PAGE_SIZE, (uintptr_t) KMEM.bitmap + i * PAGE_SIZE, KERNEL_PT_ENTRY, table);
}

void *alloc_frame(void) {
    int64_t frame = pmm_request_frame();
    if (frame > 0) {
        pmm_set_bit(frame * PAGE_SIZE);
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

void pmm_set_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        pmm_set_bit(start + PAGE_SIZE * i);
}

void pmm_clear_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        pmm_clear_bit(start + PAGE_SIZE * i);
}

void pmm_set_bit(uintptr_t address) {
    if (address == 0)
        return;
    address /= PAGE_SIZE;
    if (address > KMEM.mem_pages)
        return;
    KMEM.bitmap[address / BITMAP_ROW_BITS] |= 1UL << (address % BITMAP_ROW_BITS);
}

void pmm_clear_bit(uintptr_t address) {
    if (address == 0)
        return;
    address /= PAGE_SIZE;
    KMEM.bitmap[address / BITMAP_ROW_BITS] &= ~(1UL << (address % BITMAP_ROW_BITS));
}

bool pmm_check_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    return !(KMEM.bitmap[address / BITMAP_ROW_BITS] & (1UL << (address % BITMAP_ROW_BITS)));
}

static int64_t pmm_request_frame() {
    for (uint16_t row = 0; row < KMEM.bitmap_entries; row++) {
        if (KMEM.bitmap[row] != BITMAP_ROW_FULL)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(KMEM.bitmap[row] & (1UL << col)))
                    return row * BITMAP_ROW_BITS + col;
    }
    return -1;
}
