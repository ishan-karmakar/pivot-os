#include <mem/vmm.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
using namespace mem;

frg::manual_box<VMM> mem::kvmm;

// TODO: Place mem start after framebuffer, current only works when total physical memory is a little under 2 GB

void vmm::init() {
    kvmm.initialize(VMM::Supervisor, mem::num_pages, *mem::kmapper);
}

VMM::VMM(enum vmm_level level, size_t max_pages, PTMapper& mapper) :
    Bitmap{max_pages * PAGE_SIZE, PAGE_SIZE, map_bm(level, max_pages, mapper)}, mapper{mapper} {
    log(Info, "VMM", "Initialized VMM");
}

void *VMM::malloc(size_t size) {
    void *addr = Bitmap::malloc(size * PAGE_SIZE);
    for (size_t i = 0; i < size; i++)
        mapper.map(pmm::frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

size_t VMM::free(void *addr) {
    size_t pages = Bitmap::free(addr);
    for (size_t i = 0; i < pages; i++)
        pmm::clear(mapper.translate(reinterpret_cast<uintptr_t>(pages) + i * PAGE_SIZE));
    return pages;
}

uint8_t *VMM::map_bm(enum vmm_level level, size_t max_pages, PTMapper& mapper) {
    uintptr_t bm;
    if (level == Supervisor) {
        flags = KERNEL_PT_ENTRY;
        bm = virt_addr(num_pages * PAGE_SIZE);
    } else {
        flags = USER_PT_ENTRY;
        bm = PAGE_SIZE;
    }
    for (size_t i = 0; i < DIV_CEIL(max_pages, PAGE_SIZE); i++)
        mapper.map(pmm::frame(), bm + i * PAGE_SIZE, flags);
    return reinterpret_cast<uint8_t*>(bm);
}