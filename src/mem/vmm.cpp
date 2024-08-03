#include <mem/vmm.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
using namespace mem;

frg::manual_box<VMM> mem::kvmm;

// TODO: Place mem start after framebuffer, current only works when total physical memory is a little under 2 GB

void vmm::init() {
    kvmm.initialize(VMM::Supervisor, *mem::kmapper);
}

VMM::VMM(enum vmm_level level, PTMapper& mapper) : mapper{mapper} {
    log(INFO, "VMM", "Initialized VMM");
}

void *VMM::malloc(size_t size) {
    void *addr = nullptr;
    for (size_t i = 0; i < size; i++)
        mapper.map(pmm::frame(), reinterpret_cast<uintptr_t>(addr) + i * PAGE_SIZE, flags);
    return addr;
}

size_t VMM::free(void *addr) {
    size_t pages = 0;
    for (size_t i = 0; i < pages; i++)
        pmm::clear(mapper.translate(reinterpret_cast<uintptr_t>(pages) + i * PAGE_SIZE));
    return pages;
}
