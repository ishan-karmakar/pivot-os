#include <mem/vmm.hpp>
#include <common.h>
#include <util/logger.h>
using namespace mem;

VirtualMemoryManager::VirtualMemoryManager(enum vmm::vmm_level level, size_t max_pages, PTMapper& mapper, PhysicalMemoryManager& pmm) :
    Bitmap{max_pages * PAGE_SIZE, PAGE_SIZE, reinterpret_cast<uint8_t*>(parse_level(level))}, pmm{pmm}, mapper{mapper} {

    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        mapper.map(pmm.frame(), parse_level(level) + i * PAGE_SIZE, flags);

    Bitmap::init();
    log(Info, "VMM", "Initialized VMM");
}

void VirtualMemoryManager::post_alloc(void *start, size_t pages) {
    for (size_t i = 0; i < pages; i++)
        mapper.map(pmm.frame(), reinterpret_cast<uintptr_t>(start) + i * PAGE_SIZE, flags);
}

void VirtualMemoryManager::post_free(void *start, size_t pages) {
    for (size_t i = 0; i < pages; i++)
        pmm.clear(mapper.translate(reinterpret_cast<uintptr_t>(start) + i * PAGE_SIZE));
}

uintptr_t VirtualMemoryManager::parse_level(enum vmm::vmm_level level) {
    if (level == vmm::Supervisor) {
        flags = KERNEL_PT_ENTRY;
        return HIGHER_HALF_OFFSET;
    } else {
        flags = USER_PT_ENTRY;
        return PAGE_SIZE;
    }
}