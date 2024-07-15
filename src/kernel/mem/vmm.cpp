#include <mem/vmm.hpp>
#include <common.h>
#include <util/logger.h>
using namespace mem;

VMM::VMM(enum vmm_level level, size_t max_pages, PTMapper& mapper, PMM& pmm) :
    Bitmap{max_pages * PAGE_SIZE, PAGE_SIZE, map_bm(level, max_pages, mapper, pmm)}, pmm{pmm}, mapper{mapper} {
    log(Info, "VMM", "Initialized VMM");
}

void *VMM::alloc(size_t size) {
    void *addr = Bitmap::alloc(size * PAGE_SIZE);
    for (size_t i = 0; i < size; i++)
        mapper.map(pmm.frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

size_t VMM::free(void *addr) {
    size_t pages = Bitmap::free(addr);
    for (size_t i = 0; i < pages; i++)
        pmm.clear(mapper.translate(reinterpret_cast<uintptr_t>(pages) + i * PAGE_SIZE));
    return pages;
}

uint8_t *VMM::map_bm(enum vmm_level level, size_t max_pages, PTMapper& mapper, PMM& pmm) {
    uintptr_t bm;
    if (level == Supervisor) {
        flags = KERNEL_PT_ENTRY;
        bm = HIGHER_HALF_OFFSET;
    } else {
        flags = USER_PT_ENTRY;
        bm = PAGE_SIZE;
    }
    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        mapper.map(pmm.frame(), bm + i * PAGE_SIZE, flags);
    return reinterpret_cast<uint8_t*>(bm);
}