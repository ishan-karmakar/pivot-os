#include <mem/vmm.hpp>
#include <common.h>
#include <util/logger.h>
using namespace mem;

void VirtualMemoryManager::init(enum vmm::vmm_level level, size_t max_pages, PTMapper* mapper, PhysicalMemoryManager* pmm) {
    this->pmm = pmm;
    this->mapper = mapper;

    uintptr_t bm;
    if (level == vmm::Supervisor) {
        flags = KERNEL_PT_ENTRY;
        bm = HIGHER_HALF_OFFSET;
    } else {
        flags = USER_PT_ENTRY;
        bm = PAGE_SIZE;
    }

    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        mapper->map(pmm->frame(), bm + i * PAGE_SIZE, flags);

    Bitmap::init(max_pages * PAGE_SIZE, PAGE_SIZE, reinterpret_cast<uint8_t*>(bm));
    log(Info, "VMM", "Initialized VMM");
}

void VirtualMemoryManager::post_alloc(void *start, size_t pages) {
    while(1);
    // for (size_t i = 0; i < pages; i++);
        // mapper->map(pmm->frame(), reinterpret_cast<uintptr_t>(start) + i * PAGE_SIZE, flags);
}

void VirtualMemoryManager::post_free(void *start, size_t pages) {
    while(1);
    // for (size_t i = 0; i < pages; i++)
    //     pmm->clear(mapper->translate(reinterpret_cast<uintptr_t>(start) + i * PAGE_SIZE));
}