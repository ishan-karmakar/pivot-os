#include <mem/pmm.hpp>
#include <util/logger.h>
#include <cstring>
#include <common.h>
#include <uacpi/kernel_api.h>
using namespace mem;

uint8_t *PMM::bitmap;
size_t PMM::bitmap_size;

void PMM::init(boot_info* bi) {
    log(Info, "PMM", "Found %lu pages of physical memory (%lu mib)", bi->mem_pages, DIV_CEIL(bi->mem_pages, 256));
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
    log(Verbose, "PMM", "Bitmap: 0x%p, Size: %lu", bitmap, bitmap_size);

    memset(bitmap, 0xFF, bitmap_size);

    cur_desc = bi->mmap;
    for (size_t i = 0; i < bi->mmap_entries; i++) {
        log(Debug, "PMM", "[%lu] Type: %u - Address: 0x%p - Count: %lu", i, cur_desc->type, cur_desc->phys, cur_desc->count);
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

uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value) {
    switch (byte_width) {
    case 1:
        *out_value = *(uacpi_u8*) address;
        break;
    case 2:
        *out_value = *(uacpi_u16*) address;
        break;
    case 4:
        *out_value = *(uacpi_u32*) address;
        break;
    case 8:
        *out_value = *(uacpi_u64*) address;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 value) {
    switch (byte_width) {
    case 1:
        *(uacpi_u8*) address = value;
        break;
    case 2:
        *(uacpi_u16*) address = value;
        break;
    case 4:
        *(uacpi_u32*) address = value;
        break;
    case 8:
        *(uacpi_u64*) address = value;
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 *value) {
    return uacpi_kernel_raw_memory_read(addr, byte_width, value);
}

uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr addr, uacpi_u8 byte_width, uacpi_u64 value) {
    return uacpi_kernel_raw_memory_write(addr, byte_width, value);
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr, uacpi_size, uacpi_handle*) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_io_unmap(uacpi_handle) {
    log(Info, "uACPI", "uACPI requested to unmap io address");
}

uacpi_status uacpi_kernel_io_read(uacpi_handle, uacpi_size, uacpi_u8, uacpi_u64*) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_io_write(uacpi_handle, uacpi_size, uacpi_u8, uacpi_u64) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read(uacpi_pci_address*, uacpi_size, uacpi_u8, uacpi_u64*) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write(uacpi_pci_address*, uacpi_size, uacpi_u8, uacpi_u64) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type, uacpi_work_handler, uacpi_handle) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_wait_for_work_completion() {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_thread_id uacpi_kernel_get_thread_id() {
    log(Info, "uACPI", "uACPI requested thread id");
    return UACPI_THREAD_ID_NONE;
}