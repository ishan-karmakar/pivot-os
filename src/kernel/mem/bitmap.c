#include <mem/bitmap.h>
#include <mem/pmm.h>
#include <io/stdio.h>
#include <kernel/logging.h>
#include <drivers/framebuffer.h>
#include <sys.h>

static size_t bitmap_entries;

void init_bitmap(mem_info_t *mem_info) {
    size_t bitmap_size = mem_info->mem_pages / 8 + 1;
    bitmap_entries = mem_info->mem_pages / 64 + 1;
    for (size_t i = 0; i < bitmap_entries; i++)
        mem_info->bitmap[i] = 0;
    
    bitmap_rsv_area(PADDR((uintptr_t) mem_info->bitmap), SIZE_TO_PAGES(bitmap_size));
}

void bitmap_rsv_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        bitmap_set_bit(start + PAGE_SIZE * i);
}

void bitmap_set_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    mem_info->bitmap[address / BITMAP_ROW_BITS] |= 1 << (address % BITMAP_ROW_BITS);
}

void bitmap_clear_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    mem_info->bitmap[address / BITMAP_ROW_BITS] &= ~(1 << (address % BITMAP_ROW_BITS));
}

int64_t bitmap_request_frame() {
    for (uint16_t row = 0; row < bitmap_entries; row++) {
        if (mem_info->bitmap[row] != 0xFFFFFFFFFFFFFFFF)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(mem_info->bitmap[row] & (1 << col)))
                    return row * BITMAP_ROW_BITS + col;
    }
    return -1;
}
