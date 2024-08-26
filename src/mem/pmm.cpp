#include <mem/pmm.hpp>
#include <lib/logger.hpp>
#include <cstring>
#include <kernel.hpp>
#include <uacpi/kernel_api.h>
#include <limine.h>
#include <cstdlib>
#include <algorithm>
#define LIMINE_MMAP_ENTRY(i) mmap_request.response->entries[(i)]
using namespace pmm;

__attribute__((section(".requests")))
limine_memmap_request mmap_request = { LIMINE_MEMMAP_REQUEST, 2, nullptr };

__attribute__((section(".requests")))
limine_hhdm_request hhdm_request = { LIMINE_HHDM_REQUEST, 2, nullptr };

__attribute__((used, section(".requests")))
static limine_paging_mode_request paging_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 2,
    .response = nullptr,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .max_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .min_mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

uint8_t *bitmap;
std::size_t bitmap_size;
std::size_t ffa;
std::size_t pmm::num_pages = 0;

void pmm::init() {
    if (mmap_request.response == nullptr)
        logger::panic("PMM", "Limine failed to respond to MMAP request");

    if (hhdm_request.response == nullptr)
        logger::panic("PMM", "Limine failed to respond to HHDM request");

    std::size_t num_entries = mmap_request.response->entry_count;
    {
        std::size_t i = num_entries - 1;
        auto entry = LIMINE_MMAP_ENTRY(i);
        // I don't know if BIOS has the framebuffer in physical memory, and it might make the memory less than it should be
        for (; entry->type == LIMINE_MEMMAP_FRAMEBUFFER; entry = LIMINE_MMAP_ENTRY(--i));
        num_pages = div_ceil(entry->base + entry->length, PAGE_SIZE);
    }
    logger::verbose("PMM", "Found %lu pages of physical memory", num_pages);

    bitmap_size = div_ceil(num_pages, 8);
    for (std::size_t i = 0; i < num_entries; i++) {
        auto entry = LIMINE_MMAP_ENTRY(i);
        if (entry->type == 0 && entry->length >= bitmap_size) {
            bitmap = reinterpret_cast<uint8_t*>(virt_addr(entry->base));
            break;
        }
    }
    logger::debug("PMM", "Bitmap: %p, Size: %lu", bitmap, bitmap_size);

    memset(bitmap, 0xFF, bitmap_size);
    for (std::size_t i = 0; i < num_entries; i++) {
        auto entry = LIMINE_MMAP_ENTRY(i);
        logger::debug("PMM", "MMAP[%lu] - Base: %p, Length: %lx, Type: %lu", i, entry->base, entry->length, entry->type);
        if (entry->type == 0)
            clear(entry->base, div_ceil(entry->length, PAGE_SIZE));
    }

    set(phys_addr(reinterpret_cast<uintptr_t>(bitmap)), div_ceil(bitmap_size, PAGE_SIZE));
    set(0x8000);
    logger::info("PMM", "Finished initialization");
}

uintptr_t pmm::frame() {
    for (std::size_t off = ffa; off < (bitmap_size * 8); off++) {
        std::size_t row = off / 8, col = off % 8;
        if (off % 8 && bitmap[row] == 0xFF) continue;
        if (!(bitmap[row] & (1UL << col))) {
            off = row * 8 + col;
            ffa = off + 1;
            uintptr_t new_frame = PAGE_SIZE * off;
            set(new_frame);
            return new_frame;
        }
    }
    logger::warning("PMM", "Could not find free page in bitmap");
    return 0;
}

void pmm::clear(uintptr_t addr, std::size_t count) {
    for (std::size_t i = 0; i < count; i++)
        clear(addr + i * PAGE_SIZE);
}

void pmm::clear(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] &= ~(1 << (addr % 8));
    ffa = std::min(ffa, addr);
}

void pmm::set(uintptr_t addr, std::size_t count) {
    for (std::size_t i = 0; i < count; i++)
        set(addr + i * PAGE_SIZE);
}

void pmm::set(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] |= 1 << (addr % 8);
}