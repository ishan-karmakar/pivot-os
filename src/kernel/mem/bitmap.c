#include <mem/bitmap.h>
#include <mem/pmm.h>
#include <io/stdio.h>
#include <kernel/logging.h>
#include <drivers/framebuffer.h>
#include <sys.h>

static uint64_t *bitmap;
static size_t bitmap_entries;

void init_bitmap(mem_info_t *mem_info) {
    size_t mmap_num_entries = mem_info->mmap_size / mem_info->mmap_descriptor_size;
    mmap_descriptor_t *current_desc = mem_info->mmap;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        mem_pages += current_desc->count;
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    }

    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", mem_pages, mem_pages * PAGE_SIZE / 1048576);

    current_desc = mem_info->mmap;
    size_t bitmap_size = mem_pages / 8 + 1;
    bitmap_entries = mem_pages / 64 + 1;
    for (size_t i = 0; i < mmap_num_entries; i++) {
        if (current_desc->type == 7 && current_desc->count >= SIZE_TO_PAGES(bitmap_size)) {
            bitmap = (uint64_t*) VADDR(current_desc->physical_start);
            break;
        }
        current_desc = (mmap_descriptor_t*) ((uint8_t*) current_desc + mem_info->mmap_descriptor_size);
    }

    for (size_t i = 0; i < bitmap_entries; i++)
        bitmap[i] = 0;
    
    bitmap_rsv_area(PADDR((uintptr_t) bitmap), SIZE_TO_PAGES(bitmap_size));
}

void bitmap_rsv_area(uintptr_t start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        bitmap_set_bit(start + PAGE_SIZE * i);
}

void bitmap_set_bit(uintptr_t address) {
    address /= PAGE_SIZE;
    bitmap[address / BITMAP_ROW_BITS] |= 1 << (address % BITMAP_ROW_BITS);
}

int64_t bitmap_request_frame() {
    for (uint16_t row = 0; row < bitmap_entries; row++) {
        if (bitmap[row] != 0xFFFFFFFFFFFFFFFF)
            for (uint16_t col = 0; col < BITMAP_ROW_BITS; col++)
                if (!(bitmap[row] & (1 << col)))
                    return row * BITMAP_ROW_BITS + col;
    }
    return -1;
}
