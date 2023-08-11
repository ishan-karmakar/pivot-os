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
size_t next_available_addr;
uintptr_t hh_base;
kheap_node_t *kheap_start;
kheap_node_t *kheap_cur;
kheap_node_t *kheap_end;

kheap_node_t *create_kheap_node(kheap_node_t*, size_t);
void expand_heap(size_t);
uint8_t can_merge(kheap_node_t*);
void merge_memory_nodes(kheap_node_t*, kheap_node_t*);

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
    log(Verbose, "PMM", "Mapping virtual address %x to physical address %x", address, physical);
    log(Debug, "PMM", "PML4: %u, PDPT: %u, PD: %u", pml4_e, pdpt_e, pd_e);
    uint8_t mode = 0;

    if (!IS_HIGHER_HALF(address)) {
        mode = MEM_FLAGS_USER_LEVEL;
        flags |= MEM_FLAGS_USER_LEVEL;
    }

    if (!(p4_table[pml4_e] & PRESENT_BIT)) {
        log(Debug, "PMM", "Creating new PDPT table", pml4_e);
        uint64_t *new_table = alloc_frame();
        p4_table[pml4_e] = (uint64_t) new_table | mode | WRITE_BIT | PRESENT_BIT;
        clean_table(new_table);
    }

    uint64_t *p3_table = (uint64_t*)((p4_table[pml4_e] & PAGE_ADDR_MASK) + KERNEL_VIRTUAL_ADDR);
    if (!(p3_table[pdpt_e] & PRESENT_BIT)) {
        log(Debug, "PMM", "Creating new PD table", pdpt_e);
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
    log(Verbose, "PMM", "Kernel takes up %u pages", kernel_entries);
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
            log(Verbose, "PMM", "Reserving area starting at %x, %x bytes", mmap_entries[i].addr, mmap_entries[i].len);
            reserve_area(mmap_entries[i].addr, mmap_entries[i].len);
        }
    }
    next_available_addr = HIGHER_HALF_OFFSET + mem_size + PAGE_SIZE; // Add one page for padding
}

void pmm_map_physical_memory(void) {
    for (uint64_t addr = 0, virtual_addr = HIGHER_HALF_OFFSET; addr < mem_size; addr += PAGE_SIZE, virtual_addr += PAGE_SIZE)
        map_addr(addr, virtual_addr, WRITE_BIT | PRESENT_BIT);
    log(Info, "PMM", "Mapped physical memory");
}

void *vmm_alloc(size_t size, size_t flags) {
    size_t real_size = ALIGN_ADDR_UP(size);
    uint64_t addr = next_available_addr;
    next_available_addr += real_size;
    size_t num_pages = real_size / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++)
        map_addr((uint64_t) alloc_frame(), addr + i * PAGE_SIZE, flags);
    return (void*) addr;
}

void vmm_map_addr(uint64_t vaddress, size_t flags, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        map_addr((uint64_t) alloc_frame(), vaddress + i * PAGE_SIZE, flags);
}

void init_kheap(void) {
    kheap_start = (kheap_node_t*) vmm_alloc(PAGE_SIZE, WRITE_BIT | PRESENT_BIT);
    kheap_cur = kheap_start;
    kheap_end = kheap_start;
    kheap_start->size = PAGE_SIZE;
    kheap_cur->free = true;
    kheap_cur->next = kheap_cur->prev = NULL;
    log(Info, "KHEAP", "Initialized kernel heap");
}

void *kmalloc(size_t size) {
    kheap_node_t *cur_node = kheap_start;
    if (size == 0)
        return NULL;
    
    while (cur_node != NULL) {
        size_t real_size = KHEAP_ALIGN(size + sizeof(kheap_node_t));
        if (cur_node->free && cur_node->size >= real_size) {
            if (cur_node->size - real_size > KHEAP_MIN_ALLOC_SIZE) {
                create_kheap_node(cur_node, real_size);
                cur_node->size = real_size;
            }
            cur_node->free = false;
            return (void*) cur_node + sizeof(kheap_node_t);
        }
        if (cur_node == kheap_end) {
            expand_heap(real_size);
            if (cur_node->prev != NULL)
                cur_node = cur_node->prev;
        }
        cur_node = cur_node->next;
    }
    return NULL;
}

void expand_heap(size_t required_size) {
    size_t num_pages = ALIGN_ADDR(required_size) + PAGE_SIZE;
    uint64_t heap_end = (uint64_t) kheap_end + kheap_end->size + sizeof(kheap_node_t);
    if (heap_end > END_MEMORY)
        vmm_map_addr(heap_end, WRITE_BIT | PRESENT_BIT, num_pages);
    kheap_node_t *new_end = (kheap_node_t*) heap_end;
    new_end->next = NULL;
    new_end->prev = kheap_end;
    new_end->size = PAGE_SIZE * num_pages;
    new_end->free = true;
    kheap_end->next = new_end;
    kheap_end = new_end;

    uint8_t available_merges = can_merge(new_end);
    if (available_merges & MERGE_LEFT)
        merge_memory_nodes(new_end->prev, new_end);
}

uint8_t can_merge(kheap_node_t *cur_node) {
    kheap_node_t *prev = cur_node->prev, *next = cur_node->next;

    uint8_t available_merges = 0;
    if (prev != NULL && prev->free) {
        uint64_t prev_address = (uint64_t) prev + sizeof(kheap_node_t) + prev->size;
        if (prev_address == (uint64_t) cur_node)
            available_merges |= MERGE_LEFT;
    }
    if (next != NULL && next->free) {
        uint64_t next_address = (uint64_t) cur_node + sizeof(kheap_node_t) + cur_node->size;
        if (next_address == (uint64_t) cur_node->next)
            available_merges |= MERGE_RIGHT;
    }

    return available_merges;
}

void merge_memory_nodes(kheap_node_t *left_node, kheap_node_t *right_node) {
    if (left_node == NULL || right_node == NULL)
        return;
    if (((uint64_t) left_node + left_node->size + sizeof(kheap_node_t)) == (uint64_t) right_node) {
        left_node->size = left_node->size + right_node->size + sizeof(kheap_node_t);
        left_node->next = right_node->next;
        if (right_node->next != NULL) {
            kheap_node_t *next_node = right_node->next;
            next_node->prev = left_node;
        }
    }
}

kheap_node_t *create_kheap_node(kheap_node_t *cur_node, size_t size) {
    uint64_t header_size = sizeof(kheap_node_t);
    kheap_node_t *new_node = (kheap_node_t*) ((void*) cur_node + sizeof(kheap_node_t) + size);
    new_node->free = true;
    new_node->size = cur_node->size - (size + header_size);
    new_node->prev = cur_node;
    new_node->next = cur_node->next;
    if (cur_node->next != NULL)
        cur_node->next->prev = new_node;
    cur_node->next = new_node;
    if (cur_node == kheap_end)
        kheap_end = new_node;
    return new_node;
}

void kfree(void *ptr) {
    if (ptr == NULL)
        return;
    
    if ((uint64_t) ptr < (uint64_t) kheap_start || (uint64_t) ptr > (uint64_t) kheap_end)
        return;
    
    kheap_node_t *cur_node = kheap_start;
    while (cur_node != NULL) {
        if (((uint64_t) cur_node + sizeof(kheap_node_t)) == (uint64_t) ptr) {
            cur_node->free = true;
            uint8_t available_merges = can_merge(cur_node);
            if (available_merges & MERGE_RIGHT)
                merge_memory_nodes(cur_node, cur_node->next);
            if (available_merges & MERGE_LEFT)
                merge_memory_nodes(cur_node->prev, cur_node);
            return;
        }
        cur_node = cur_node->next;
    }
}