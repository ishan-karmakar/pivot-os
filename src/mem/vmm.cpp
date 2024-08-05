#define BUDDY_ALLOC_IMPLEMENTATION
#include <mem/vmm.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <limine.h>

using namespace vmm;

extern volatile limine_memmap_request mmap_request;
frg::manual_box<VMM> vmm::kvmm;

void vmm::init() {
    auto last_ent = mmap_request.response->entries[mmap_request.response->entry_count - 1];
    uintptr_t start = virt_addr(last_ent->base + last_ent->length);
    kvmm.initialize(start, pmm::num_pages * PAGE_SIZE, mapper::KERNEL_ENTRY, *mapper::kmapper);
}

VMM::VMM(uintptr_t start, size_t size, size_t flags, mapper::PTMapper& mpr) : flags{flags}, mpr{mpr} {
    size_t metadata_pages = div_ceil(buddy_sizeof_alignment(pmm::num_pages * PAGE_SIZE, PAGE_SIZE), PAGE_SIZE);
    for (size_t i = 0; i < metadata_pages; i++)
        mpr.map(pmm::frame(), start + i * PAGE_SIZE, flags);
    uint8_t *at = reinterpret_cast<uint8_t*>(start);
    buddy = buddy_init_alignment(at, at + metadata_pages * PAGE_SIZE, size - metadata_pages * PAGE_SIZE, PAGE_SIZE);
    log(INFO, "VMM", "Initialized VMM");
}

void *VMM::malloc(size_t size) {
    void *addr = buddy_malloc(buddy, size);
    for (size_t i = 0; i < div_ceil(size, PAGE_SIZE); i++)
        mpr.map(pmm::frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

void VMM::free(void *addr) {
    buddy_free(buddy, addr);
}
