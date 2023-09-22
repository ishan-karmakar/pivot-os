#include <stddef.h>
#include <kernel/multiboot.h>
#include <sys.h>
#include <mem/bitmap.h>

mb_mmap_entry_t *mmap_entries;
uint32_t mmap_num_entries;
extern size_t mem_size;
uint64_t *memory_map;
uint32_t bitmap_size;
uint32_t used_frames;
uint32_t num_entries;
uint64_t mmap_phys_addr;

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

uint64_t get_bitmap_region(uint64_t lower_limit, size_t bytes_needed) {
    for (uint32_t i = 0; i < mmap_num_entries; i++) {
        mb_mmap_entry_t *entry = &mmap_entries[i];
        if (entry->type != 1) // 4 = AVAILABLE
            continue;
        if (entry->addr + entry->len < lower_limit)
            continue;
        size_t entry_offset = lower_limit > entry->addr ? lower_limit - entry->addr : 0;
        size_t available_space = entry->len - entry_offset;
        if (available_space >= bytes_needed)
            return entry->addr + entry_offset;
    }
    return 0;
}

void bitmap_rsv_area(uint64_t start, size_t size) {
    uint64_t location = start / PAGE_SIZE;
    uint32_t num_frames = size / PAGE_SIZE;
    if (size % PAGE_SIZE)
        num_frames++;
    for (; num_frames > 0; num_frames--) {
        bitmap_set_bit(location++);
        used_frames++;
    }
}

uint32_t get_kernel_entries(uint64_t kernel_end) {
    uint32_t kernel_entries = kernel_end / PAGE_SIZE + 1;
    if (kernel_end % PAGE_SIZE)
        return kernel_entries + 1;
    return kernel_entries;
}

void initialize_bitmap(uint64_t rsv_end, mb_mmap_entry_t *me, uint32_t mne) {
    mmap_entries = me;
    mmap_num_entries = mne;
    // Bitmap is stored in 64 bit chunks, so this num_entries is number of those chunks
    // Actual size of bitmap is (size / 8 + 1)
    bitmap_size = mem_size / PAGE_SIZE + 1;
    num_entries = bitmap_size / 64 + 1;
    mmap_phys_addr = get_bitmap_region(rsv_end, bitmap_size / 8 + 1);
    uint64_t end_physical_memory = END_MAPPED_MEMORY - KERNEL_VIRTUAL_ADDR;
    if (mmap_phys_addr > end_physical_memory) {
        // map_addr(ALIGN_ADDR(mmap_phys_addr), mmap_phys_addr + KERNEL_VIRTUAL_ADDR, PRESENT_BIT | WRITE_BIT);
    }
    memory_map = (uint64_t*) (mmap_phys_addr + KERNEL_VIRTUAL_ADDR);
    for (uint32_t i = 0; i < num_entries; i++)
        memory_map[i] = 0;
    uint32_t kernel_entries = get_kernel_entries(rsv_end);
    uint32_t num_bitmap_rows = kernel_entries / 64;
    uint32_t i = 0;
    for (i = 0; i < num_bitmap_rows; i++)
        memory_map[i] = ~0;
    memory_map[i] = ~(~(0ul) << (kernel_entries - num_bitmap_rows * 64));
    used_frames = kernel_entries;

    bitmap_rsv_area(mmap_phys_addr, bitmap_size / 8 + 1);
}