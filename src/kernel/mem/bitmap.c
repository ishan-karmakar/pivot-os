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
    
    log(Info, "BITMAP", "Initialized bitmap");
}

void bitmap_rsv_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        bitmap_set_bit(start + PAGE_SIZE * i);
}

void bitmap_clear_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        bitmap_clear_bit(start + PAGE_SIZE * i);
}

void bitmap_set_bit(uintptr_t address) {
    if (!addr_in_phys_mem(address))
        return;
    address /= PAGE_SIZE;
    mem_info->bitmap[address / BITMAP_ROW_BITS] |= 1UL << (address % BITMAP_ROW_BITS);
}

void bitmap_clear_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    mem_info->bitmap[address / BITMAP_ROW_BITS] &= ~(1UL << (address % BITMAP_ROW_BITS));
}

int64_t bitmap_request_frame() {
    for (uint16_t row = 0; row < mem_info->bitmap_entries; row++) {
        // log(Verbose, "BITMAP", "%b", mem_info->bitmap[row]);
        if (mem_info->bitmap[row] != BITMAP_ROW_FULL)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(mem_info->bitmap[row] & (1 << col))) {
                    // printf("\n");
                    return row * BITMAP_ROW_BITS + col;
                }
    }
    return -1;
}
