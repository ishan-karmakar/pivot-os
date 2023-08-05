#include <stddef.h>
#include <mem/mem.h>
#include <kernel/multiboot.h>
#include <kernel/logging.h>

extern uint64_t p4_table[512];
extern size_t mem_size;
uint32_t bitmap_size;
uint32_t used_frames;
uint32_t num_entries;
uint32_t mmap_num_entries;
mb_mmap_entry_t *mmap_entries;
uint64_t mmap_phys_addr;
uint64_t *memory_map;

const char *mmap_types[] = {
    "Invalid",
    "Available",
    "Reserved",
    "Reclaimable",
    "NVS",
    "Defective"
};

int64_t bitmap_request_frame(void) {
    for (uint16_t row = 0; row < num_entries; row++)
        if (memory_map[row] != BITMAP_ENTRY_FULL)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(memory_map[row] & (1 << col)))
                    return row * BITMAP_ROW_BITS + col;
    return -1;
}

void bitmap_set_bit(uint64_t location) {
    memory_map[location / BITMAP_ROW_BITS] |= 1 << (location % BITMAP_ROW_BITS);
}

void bitmap_set_bit_addr(uint64_t address) {
    if (address < mem_size)
        bitmap_set_bit(address / PAGE_SIZE);
}

void *alloc_frame(void) {
    if (used_frames >= bitmap_size)
        return NULL;
    uint64_t frame = bitmap_request_frame();
    if (frame > 0) {
        bitmap_set_bit(frame);
        used_frames++;
        return (void*)(frame * PAGE_SIZE);
    }

    return NULL;
}

void clean_table(uint64_t *table) {
    for (int i = 0; i < PAGES_PER_TABLE; i++)
        table[i] = 0;
}

void *map_addr(uint64_t physical, uint64_t address, size_t flags) {
    uint16_t pml4_e = PML4_ENTRY(address);
    uint16_t pdpt_e = PDPR_ENTRY(address);
    uint16_t pd_e = PD_ENTRY(address);
    log(Verbose, "MEM", "Mapping virtual address %x to physical address %x", address, physical);
    log(Debug, "MEM", "PML4: %u, PDPT: %u, PD: %u", pml4_e, pdpt_e, pd_e);
    uint8_t mode = 0;

    if (!IS_HIGHER_HALF(address)) {
        mode = MEM_FLAGS_USER_LEVEL;
        flags |= MEM_FLAGS_USER_LEVEL;
    }

    if (!(p4_table[pml4_e] & 1)) {
        log(Debug, "MEM", "Creating new PDPT table", pml4_e);
        uint64_t *new_table = alloc_frame();
        p4_table[pml4_e] = (uint64_t) new_table | mode | WRITE_BIT | PRESENT_BIT;
        clean_table(new_table);
    }

    uint64_t *p3_table = (uint64_t*)((p4_table[pml4_e] & PAGE_ADDR_MASK) + KERNEL_VIRTUAL_ADDR);
    if (!(p3_table[pdpt_e] & 1)) {
        log(Debug, "MEM", "Creating new PD table", pdpt_e);
        uint64_t *new_table = (uint64_t*) alloc_frame();
        p3_table[pdpt_e] = (uint64_t) new_table | mode | WRITE_BIT | PRESENT_BIT;
        clean_table(new_table);
    }

    uint64_t *p2_table = (uint64_t*)((p3_table[pdpt_e] & PAGE_ADDR_MASK) + KERNEL_VIRTUAL_ADDR);
    p2_table[pd_e] = physical | HUGEPAGE_BIT | flags;
    return (void*) address;
}

void mmap_parse(mb_mmap_t *root) {
    mmap_num_entries = (root->size - sizeof(mb_mmap_t)) / root->entry_size;
    mmap_entries = (mb_mmap_entry_t*)(root + 1);
    for (uint32_t i = 0; i < mmap_num_entries; i++) {
        mb_mmap_entry_t *entry = &mmap_entries[i];
        log(Verbose, "MMAP", "[%d] Address: %x, Len: %x, Type: %s", i, entry->addr, entry->len, mmap_types[entry->type]);
    }
}

uint32_t get_kernel_entries(uint64_t kernel_end) {
    uint32_t kernel_entries = kernel_end / PAGE_SIZE + 1;
    if (kernel_end % PAGE_SIZE)
        return kernel_entries + 1;
    return kernel_entries;
}

uint64_t get_bitmap_region(uint64_t lower_limit, size_t bytes_needed) {
    for (uint32_t i = 0; i < mmap_num_entries; i++) {
        mb_mmap_entry_t *entry = &mmap_entries[i];
        if (entry->type != 1) // 4 = AVAILABLE
            continue;
        if (entry->addr + entry->len < lower_limit)
            continue;
        size_t entry_offset = lower_limit > entry->addr ? lower_limit - entry->addr : 0;
        size_t available_space = entry->len - entry_offset;
        if (available_space >= bytes_needed) {
            log(Verbose, "BITMAP", "Found space for bitmap at address: %x, size: %x", entry->addr + entry_offset, bytes_needed);
            return entry->addr + entry_offset;
        }
    }
    log(Error, "BITMAP", "Couldn't find space to fit bitmap");
    return 0;
}

void reserve_area(uint64_t start, size_t size) {
    uint64_t location = start / PAGE_SIZE;
    uint32_t num_frames = size / PAGE_SIZE;
    if (size % PAGE_SIZE)
        num_frames++;
    for (; num_frames > 0; num_frames--) {
        bitmap_set_bit(location++);
        used_frames++;
    }
}

void initialize_bitmap(uint64_t rsv_end, uint64_t mem_size) {
    // Bitmap is stored in 64 bit chunks, so this num_entries is number of those chunks
    // Actual size of bitmap is (size / 8 + 1)
    bitmap_size = mem_size / PAGE_SIZE + 1;
    num_entries = bitmap_size / 64 + 1;
    mmap_phys_addr = get_bitmap_region(rsv_end, bitmap_size / 8 + 1);
    uint64_t end_physical_memory = END_MEMORY - KERNEL_VIRTUAL_ADDR;
    if (mmap_phys_addr > end_physical_memory) {
        log(Verbose, "BITMAP", "The address %x is above the initially mapped memory: %x", mmap_phys_addr, end_physical_memory);
        // map_addr(ALIGN_ADDR(mmap_phys_addr), mmap_phys_addr + KERNEL_VIRTUAL_ADDR, PRESENT_BIT | WRITE_BIT);
    } else {
        log(Verbose, "BITMAP", "The address %x is not above the initially mapped memory: %x", mmap_phys_addr, end_physical_memory);
    }
    memory_map = (uint64_t*) (mmap_phys_addr + KERNEL_VIRTUAL_ADDR);
    for (uint32_t i = 0; i < num_entries; i++)
        memory_map[i] = 0;
    uint32_t kernel_entries = get_kernel_entries(rsv_end);
    log(Verbose, "MEM", "Kernel takes up %u pages", kernel_entries);
    uint32_t num_bitmap_rows = kernel_entries / 64;
    uint32_t i = 0;
    for (i = 0; i < num_bitmap_rows; i++)
        memory_map[i] = ~0;
    memory_map[i] = ~(~(0ul) << (kernel_entries - num_bitmap_rows * 64));
    used_frames = kernel_entries;
}

void init_pmm(uintptr_t addr, uint32_t size, uint64_t mem_size) {
    initialize_bitmap(addr + size, mem_size);
    reserve_area(mmap_phys_addr, bitmap_size / 8 + 1);
    for (uint32_t i = 0; i < mmap_num_entries; i++) {
        if (mmap_entries[i].addr < mem_size && mmap_entries[i].type != 1) {
            log(Verbose, "MEM", "Reserving area starting at %x, %x bytes", mmap_entries[i].addr, mmap_entries[i].len);
            reserve_area(mmap_entries[i].addr, mmap_entries[i].len);
        }
    }
}

void init_vmm(void) {}