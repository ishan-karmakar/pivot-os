#include <mem/bitmap.h>
#include <mem/pmm.h>
#include <io/stdio.h>
#include <kernel/logging.h>
#include <drivers/framebuffer.h>
#include <sys.h>

// TODO: Find another way to store this data to save space
void init_bitmap(void) {
    for (size_t i = 0; i < mem_info->bitmap_entries; i++)
        mem_info->bitmap[i] = BITMAP_ROW_FULL;
    
    size_t mmap_num_entries = mem_info->mmap_size / mem_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = mem_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        log(Debug, "MMAP", "[%u] Type: %u - Address: %x - Num Pages: %u", i, current_desc->type, current_desc->physical_start, current_desc->count);
        uint8_t type = current_desc->type;
        if ((type == 3 || type == 4 || type == 7) && current_desc->physical_start != 0)
            bitmap_clear_area(current_desc->physical_start, current_desc->count);
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    } // IDX: 3, Col: 2

    bitmap_rsv_area(PADDR((uintptr_t) mem_info->bitmap), mem_info->bitmap_size);
    log(Info, "BITMAP", "Initialized bitmap");
}

void bitmap_rsv_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++) {
        bitmap_set_bit(start + PAGE_SIZE * i);
    }
}

void bitmap_clear_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++) {
        bitmap_clear_bit(start + PAGE_SIZE * i);
    }
}

void bitmap_set_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    if (address > mem_info->mem_pages)
        return;
    mem_info->bitmap[address / BITMAP_ROW_BITS] |= 1UL << (address % BITMAP_ROW_BITS);
}

void bitmap_clear_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    mem_info->bitmap[address / BITMAP_ROW_BITS] &= ~(1UL << (address % BITMAP_ROW_BITS));
}

int64_t bitmap_request_frame() {
    for (uint16_t row = 0; row < mem_info->bitmap_entries; row++) {
        if (mem_info->bitmap[row] != BITMAP_ROW_FULL)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(mem_info->bitmap[row] & (1 << col)))
                    return row * BITMAP_ROW_BITS + col;
    }
    return -1;
}
