#include <mem/pmm.hpp>
#include <util/logger.h>
#include <libc/string.h>
#include <common.h>
using namespace mem;

PMM::PMM(boot_info* bi) {
    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", bi->mem_pages, DIV_CEIL(bi->mem_pages, 256));
    mmap_desc *cur_desc = bi->mmap;
    bitmap_size = DIV_CEIL(bi->mem_pages, 8);
    for (size_t i = 0; i < bi->mmap_entries; i++) {
        uint32_t type = cur_desc->type;
        if ((type >= 3 && type <= 7) && cur_desc->phys != 0 && cur_desc->count >= DIV_CEIL(bitmap_size, PAGE_SIZE)) {
            bitmap = reinterpret_cast<uint8_t*>(cur_desc->phys);
            break;
        }

        cur_desc = reinterpret_cast<mmap_desc*>(reinterpret_cast<uint8_t*>(cur_desc) + bi->desc_size);
    }
    log(Verbose, "PMM", "Bitmap: %x, Size: %u", bitmap, bitmap_size);

    memset(bitmap, 0xFF, bitmap_size);

    cur_desc = bi->mmap;
    for (size_t i = 0; i < bi->mmap_entries; i++) {
        log(Debug, "PMM", "[%u] Type: %u - Address: %x - Count: %u", i, cur_desc->type, cur_desc->phys, cur_desc->count);
        uint32_t type = cur_desc->type;
        if ((type >= 3 && type <= 7) && cur_desc->phys != 0) {
            clear(cur_desc->phys, cur_desc->count);
        }

        cur_desc = reinterpret_cast<mmap_desc*>(reinterpret_cast<uint8_t*>(cur_desc) + bi->desc_size);
    }

    set(reinterpret_cast<uintptr_t>(bitmap), DIV_CEIL(bitmap_size, PAGE_SIZE));
    set(0x8000);
    log(Info, "PMM", "Initialized PMM");
}

uintptr_t PMM::frame() {
    for (uint16_t row = 0; row < bitmap_size; row++)
        if (bitmap[row] != 0xFF)
            for (uint16_t col = 0; col < 8; col++)
                if (!(bitmap[row] & (1UL << col))) {
                    uintptr_t new_frame = PAGE_SIZE * (row * 8 + col);
                    set(new_frame);
                    return new_frame;
                }
    log(Warning, "PMM", "Could not find free page in bitmap");
    return 0;
}

void PMM::clear(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        clear(addr + i * PAGE_SIZE);
}

void PMM::clear(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] &= ~(1 << (addr % 8));
}

void PMM::set(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        set(addr + i * PAGE_SIZE);
}

void PMM::set(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] |= 1 << (addr % 8);
}
