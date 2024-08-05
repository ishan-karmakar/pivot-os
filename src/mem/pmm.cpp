#include <mem/pmm.hpp>
#include <util/logger.hpp>
#include <cstring>
#include <kernel.hpp>
#include <uacpi/kernel_api.h>
#include <limine.h>
#include <cstdlib>
#define LIMINE_MMAP_ENTRY(i) mmap_request.response->entries[(i)]
using namespace pmm;

__attribute__((section(".requests")))
volatile limine_memmap_request mmap_request = { LIMINE_MEMMAP_REQUEST, 2, nullptr };

__attribute__((section(".requests")))
static volatile limine_hhdm_request hhdm_request = { LIMINE_HHDM_REQUEST, 2, nullptr };

__attribute__((used, section(".requests")))
static volatile limine_paging_mode_request paging_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 2,
    .response = nullptr,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .max_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .min_mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

uint8_t *bitmap;
size_t bitmap_size;
size_t pmm::num_pages = 0;

void pmm::init() {
    if (mmap_request.response == nullptr)
        panic("PMM", "Limine failed to respond to MMAP request");

    if (hhdm_request.response == nullptr)
        panic("PMM", "Limine failed to respond to HHDM request");

    size_t num_entries = mmap_request.response->entry_count;
    {
        size_t i = num_entries - 1;
        auto entry = LIMINE_MMAP_ENTRY(i);
        // I don't know if BIOS has the framebuffer in physical memory, and it might make the memory less than it should be
        for (; entry->type == LIMINE_MEMMAP_FRAMEBUFFER; entry = LIMINE_MMAP_ENTRY(--i));
        num_pages = div_ceil(entry->base + entry->length, PAGE_SIZE);
    }
    log(INFO, "PMM", "Found %lu pages of physical memory", num_pages);

    bitmap_size = div_ceil<size_t>(num_pages, 8);
    for (size_t i = 0; i < num_entries; i++) {
        auto entry = LIMINE_MMAP_ENTRY(i);
        if (entry->type == 0 && entry->length >= bitmap_size) {
            bitmap = reinterpret_cast<uint8_t*>(virt_addr(entry->base));
            break;
        }
    }
    log(VERBOSE, "PMM", "Bitmap: %p, Size: %lu", bitmap, bitmap_size);

    memset(bitmap, 0xFF, bitmap_size);
    for (size_t i = 0; i < num_entries; i++) {
        auto entry = LIMINE_MMAP_ENTRY(i);
        log(DEBUG, "PMM", "MMAP[%lu] - Base: %p, Length: %lx, Type: %lu", i, entry->base, entry->length, entry->type);
        if (entry->type == 0)
            clear(entry->base, div_ceil(entry->length, PAGE_SIZE));
    }

    set(phys_addr(reinterpret_cast<uintptr_t>(bitmap)), div_ceil(bitmap_size, PAGE_SIZE));
    set(0x8000);
    log(INFO, "PMM", "Initialized PMM");
}

uintptr_t pmm::frame() {
    for (uint16_t row = 0; row < bitmap_size; row++)
        if (bitmap[row] != 0xFF)
            for (uint16_t col = 0; col < 8; col++)
                if (!(bitmap[row] & (1UL << col))) {
                    uintptr_t new_frame = PAGE_SIZE * (row * 8 + col);
                    set(new_frame);
                    return new_frame;
                }
    log(WARNING, "PMM", "Could not find free page in bitmap");
    return 0;
}

void pmm::clear(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        clear(addr + i * PAGE_SIZE);
}

void pmm::clear(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] &= ~(1 << (addr % 8));
}

void pmm::set(uintptr_t addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        set(addr + i * PAGE_SIZE);
}

void pmm::set(uintptr_t addr) {
    addr /= PAGE_SIZE;
    if (addr >= (bitmap_size * 8))
        return;
    bitmap[addr / 8] |= 1 << (addr % 8);
}

uintptr_t virt_addr(uintptr_t phys) {
    return phys + hhdm_request.response->offset;
}

uintptr_t phys_addr(uintptr_t virt) {
    return virt - hhdm_request.response->offset;
}

uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value) {
    switch (byte_width) {
    case 1:
        *out_value = *(volatile uacpi_u8*) address;
        break;
    case 2:
        *out_value = *(volatile uacpi_u16*) address;
        break;
    case 4:
        *out_value = *(volatile uacpi_u32*) address;
        break;
    case 8:
        *out_value = *(volatile uacpi_u64*) address;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 value) {
    switch (byte_width) {
    case 1:
        *(volatile uacpi_u8*) address = value;
        break;
    case 2:
        *(volatile uacpi_u16*) address = value;
        break;
    case 4:
        *(volatile uacpi_u32*) address = value;
        break;
    case 8:
        *(volatile uacpi_u64*) address = value;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 *value) {
    uint16_t p = addr;
    switch (byte_width) {
    case 1: {
        uint8_t v;
        asm ("inb %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    } case 2: {
        uint16_t v;
        asm ("inw %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    } case 4: {
        uint32_t v;
        asm ("inl %1, %0" : "=a" (v) : "d" (p));
        *value = v;
        break;
    }
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 value) {
    uint16_t p = addr;
    switch (byte_width) {
    case 1: {
        uint8_t v = value;
        asm ("outb %0, %1" : : "a" (v), "d" (p));
        break;
    } case 2: {
        uint16_t v = value;
        asm ("outw %0, %1" : : "a" (v), "d" (p));
        break;
    } case 4: {
        uint32_t v = value;
        asm ("outl %0, %1" : : "a" (v), "d" (p));
        break;
    }
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr, uacpi_size, uacpi_handle*) {
    log(VERBOSE, "uACPI", "io_map");
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_io_unmap(uacpi_handle) {
    log(INFO, "uACPI", "uACPI requested to unmap io address");
}

uacpi_status uacpi_kernel_io_read(uacpi_handle, uacpi_size, uacpi_u8, uacpi_u64*) {
    log(VERBOSE, "uACPI", "io_read");
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_io_write(uacpi_handle, uacpi_size, uacpi_u8, uacpi_u64) {
    log(VERBOSE, "uACPI", "io_write");
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read(uacpi_pci_address*, uacpi_size, uacpi_u8, uacpi_u64*) {
    log(VERBOSE, "uACPI", "pci_read");
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write(uacpi_pci_address*, uacpi_size, uacpi_u8, uacpi_u64) {
    log(VERBOSE, "uACPI", "pci_write");
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type, uacpi_work_handler, uacpi_handle) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_wait_for_work_completion() {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_thread_id uacpi_kernel_get_thread_id() {
    log(INFO, "uACPI", "uACPI requested thread id");
    return UACPI_THREAD_ID_NONE;
}