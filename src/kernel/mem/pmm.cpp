#include <mem/pmm.hpp>
#include <util/logger.h>
#include <libc/string.h>
#include <common.h>
using namespace mem;

void PhysicalMemoryManager::init(struct boot_info* bi) {
    log(Info, "PMM", "Found %u pages of physical memory (%u mib)", bi->mem_pages, DIV_CEIL(bi->mem_pages, 256));
    struct mmap_desc *cur_desc = bi->mmap;
    bitmap_size = DIV_CEIL(bi->mem_pages, 8);
    for (size_t i = 0; i < bi->mmap_entries; i++) {
        uint32_t type = cur_desc->type;
        if ((type >= 3 && type <= 7) && cur_desc->phys != 0 && cur_desc->count >= DIV_CEIL(bitmap_size, PAGE_SIZE)) {
            bitmap = reinterpret_cast<uint8_t*>(cur_desc->phys);
            break;
        }

        cur_desc = reinterpret_cast<struct mmap_desc*>(reinterpret_cast<uint8_t*>(cur_desc) + bi->desc_size);
    }
    log(Verbose, "PMM", "Bitmap: %x, Size: %u", bitmap, bitmap_size);
    
    cur_desc = bi->mmap;
    for (size_t i = 0; i < bi->mmap_entries; i++) {
        log(Debug, "PMM", "[%u] Type: %u - Address: %x - Count: %u", i, cur_desc->type, cur_desc->phys, cur_desc->count);
        uint32_t type = cur_desc->type;
        if ((type >= 3 && type <= 7) && cur_desc->phys != 0) {
            log(Debug, "PMM", "Clearing region start at %x, spanning %u pages", cur_desc->phys, cur_desc->count);
            clear_region(cur_desc->phys, cur_desc->count);
        }

        cur_desc = reinterpret_cast<struct mmap_desc*>(reinterpret_cast<uint8_t*>(cur_desc) + bi->desc_size);
    }

    set_region(reinterpret_cast<uintptr_t>(bitmap), DIV_CEIL(bitmap_size, PAGE_SIZE));
    set_region(0x8000);
    log(Info, "PMM", "Initialized PMM");
}

void PhysicalMemoryManager::clear_region(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        clear_region(addr + i * PAGE_SIZE);
}

void PhysicalMemoryManager::clear_region(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        log(Warning, "PMM", "Address exceeds bitmap size (%u >= %u)", addr, bitmap_size);
    bitmap[addr / 8] &= ~(1 << (addr % 8));
}

void PhysicalMemoryManager::set_region(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        set_region(addr + i * PAGE_SIZE);
}

void PhysicalMemoryManager::set_region(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        log(Warning, "PMM", "Address exceeds bitmap size (%u >= %u)", addr, bitmap_size);
    bitmap[addr / 8] |= 1 << (addr % 8);
}
