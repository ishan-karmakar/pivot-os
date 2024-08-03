#include <mem/vmm.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <limine.h>
using namespace mem;

extern volatile limine_memmap_request mmap_request;
frg::manual_box<VMM> mem::kvmm;

void vmm::init() {
    auto last_ent = mmap_request.response->entries[mmap_request.response->entry_count - 1];
    uintptr_t start = virt_addr(last_ent->base + last_ent->length);
    kvmm.initialize(start, KERNEL_PT_ENTRY, *mem::kmapper);
}

VMM::VMM(uintptr_t start, size_t flags, PTMapper& mapper) : nodes{(decltype(nodes)) start}, flags{flags}, mapper{mapper} {
    mapper.map(pmm::frame(), start, KERNEL_PT_ENTRY);
    node *n = new(nodes->nodes) node;
    n->base = start;
    n->length = PAGE_SIZE;
    tree.insert(n);
    nodes->num_nodes = nodes->num_pages = 1;
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

VMM::node *VMM::new_node() {
    for (uint64_t i = 0; i < nodes->num_nodes; i++)
        if (nodes->nodes[i].base & 1) // If special bit in base marks node as free, then return it
            return nodes->nodes + i;
}
