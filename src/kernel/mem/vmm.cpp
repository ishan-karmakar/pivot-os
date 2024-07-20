#include <mem/vmm.hpp>
#include <common.h>
#include <util/logger.h>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
using namespace mem;

VMM::VMM(enum vmm_level level, size_t max_pages, PTMapper& mapper) :
    Bitmap{max_pages * PAGE_SIZE, PAGE_SIZE, map_bm(level, max_pages, mapper)}, mapper{mapper} {
    log(Info, "VMM", "Initialized VMM");
}

void *VMM::malloc(size_t size) {
    void *addr = Bitmap::malloc(size * PAGE_SIZE);
    for (size_t i = 0; i < size; i++)
        mapper.map(PMM::frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

size_t VMM::free(void *addr) {
    size_t pages = Bitmap::free(addr);
    for (size_t i = 0; i < pages; i++)
        PMM::clear(mapper.translate(reinterpret_cast<uintptr_t>(pages) + i * PAGE_SIZE));
    return pages;
}

uint8_t *VMM::map_bm(enum vmm_level level, size_t max_pages, PTMapper& mapper) {
    uintptr_t bm;
    if (level == Supervisor) {
        flags = KERNEL_PT_ENTRY;
        bm = HIGHER_HALF_OFFSET;
    } else {
        flags = USER_PT_ENTRY;
        bm = PAGE_SIZE;
    }
    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        mapper.map(PMM::frame(), bm + i * PAGE_SIZE, flags);
    return reinterpret_cast<uint8_t*>(bm);
}