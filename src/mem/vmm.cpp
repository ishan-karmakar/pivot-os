#define BUDDY_ALLOC_IMPLEMENTATION
#include <mem/vmm.hpp>
#include <kernel.hpp>
#include <lib/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <limine.h>

using namespace vmm;

extern volatile limine_memmap_request mmap_request;
frg::manual_box<vmm::vmm_t> vmm::kvmm;

void vmm::init() {
    auto last_ent = mmap_request.response->entries[mmap_request.response->entry_count - 1];
    uintptr_t start = virt_addr(last_ent->base + last_ent->length);
    kvmm.initialize(start, pmm::num_pages * PAGE_SIZE, mapper::KERNEL_ENTRY, *mapper::kmapper);
    logger::info("VMM", "Initialized VMM");
}

vmm::vmm_t::vmm_t(uintptr_t start, std::size_t size, std::size_t flags, mapper::ptmapper& mpr) : flags{flags}, mpr{mpr} {
    std::size_t metadata_pages = div_ceil(buddy_sizeof_alignment(size, PAGE_SIZE), PAGE_SIZE);
    for (std::size_t i = 0; i < metadata_pages; i++)
        mpr.map(pmm::frame(), start + i * PAGE_SIZE, flags);
    uint8_t *at = reinterpret_cast<uint8_t*>(start);
    buddy = buddy_init_alignment(at, at + metadata_pages * PAGE_SIZE, size, PAGE_SIZE);
}

vmm::vmm_t::~vmm_t() {
    auto buddy_callback = [](void *self_void, void *addr, std::size_t size, std::size_t a) -> void* {
        if (a) {
            auto self = reinterpret_cast<vmm_t*>(self_void);
            for (std::size_t i = 0; i < div_ceil(size, PAGE_SIZE); i++)
                pmm::clear(self->mpr.translate(reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE));
        }
        return nullptr;
    };
    buddy_walk(buddy, buddy_callback, this);
    uintptr_t start = reinterpret_cast<uintptr_t>(buddy);
    uintptr_t metadata_pages = div_ceil(buddy_sizeof_alignment(buddy->memory_size, PAGE_SIZE), PAGE_SIZE);
    for (std::size_t i = 0; i < metadata_pages; i++)
        pmm::clear(mpr.translate(start + i * PAGE_SIZE));
    mpr.unmap(start, div_ceil(buddy->memory_size, PAGE_SIZE));
    delete &mpr;
}

void *vmm::vmm_t::malloc(std::size_t size) {
    void *addr = buddy_malloc(buddy, size);
    for (std::size_t i = 0; i < div_ceil(size, PAGE_SIZE); i++)
        mpr.map(pmm::frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

void vmm::vmm_t::free(void *addr) {
    auto buddy_callback = [](void *taddr, void *addr, std::size_t size, std::size_t a) -> void* {
        if (taddr == addr && a)
            return reinterpret_cast<void*>(size);
        return nullptr;
    };

    std::size_t pages = div_ceil(reinterpret_cast<std::size_t>(buddy_walk(buddy, buddy_callback, addr)), PAGE_SIZE);
    buddy_free(buddy, addr);
    for (std::size_t i = 0; i < pages; i++)
        mpr.unmap(reinterpret_cast<std::size_t>(addr), pages);
}
