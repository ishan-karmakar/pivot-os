#include <mem/heap.hpp>
#include <lib/logger.hpp>
#include <mem/vmm.hpp>
#include <uacpi/kernel_api.h>
#include <atomic>
#include <kernel.hpp>
#include <cstring>
#include <frg/manual_box.hpp>
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <frg/allocation.hpp>
using namespace frg;
using namespace heap;

frg::manual_box<policy> heap_slab_policy;
frg::manual_box<frg::slab_pool<policy, frg::simple_spinlock>> heap_pool;

uintptr_t policy::map(std::size_t size) {
    return reinterpret_cast<uintptr_t>(vmm.malloc(round_up(size, PAGE_SIZE)));;
}

void policy::unmap(uintptr_t addr, std::size_t) {
    vmm.free(reinterpret_cast<void*>(addr));
};

allocator_t::allocator_t() : frg::slab_allocator<policy, frg::simple_spinlock>{heap_pool.get()} {}

void heap::init() {
    heap_slab_policy.initialize(*vmm::kvmm);
    heap_pool.initialize(*heap_slab_policy);
    logger::info("HEAP[INIT]", "Initialized slab allocator");
}

allocator_t& heap::allocator() {
    static allocator_t alloc;
    return alloc;
}

ALIAS_FN(term_alloc)
ALIAS_FN(uacpi_kernel_alloc)
[[gnu::noinline]] void *malloc(std::size_t size) {
    return heap::allocator().allocate(size);
}

ALIAS_FN(uacpi_kernel_calloc);
[[gnu::noinline]] void *calloc(std::size_t count, std::size_t size) {
    size *= count;
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

ALIAS_FN(term_realloc);
[[gnu::noinline]] void *realloc(void *old, std::size_t size) {
    return heap::allocator().reallocate(old, size);
}

ALIAS_FN(term_free);
ALIAS_FN(term_freensz);
ALIAS_FN(uacpi_kernel_free);
[[gnu::noinline]] void free(void *ptr) {
    return heap::allocator().free(ptr);
}

void *operator new(std::size_t size) {
    auto t = malloc(size);
    return t;
}

void operator delete(void *ptr) {
    return free(ptr);
}

void operator delete(void *ptr, std::size_t) {
    return operator delete(ptr);
}

void *operator new[](std::size_t size) {
    return operator new(size);
}

void operator delete[](void *ptr) {
    return operator delete(ptr);
}

void operator delete[](void *ptr, std::size_t) {
    return operator delete[](ptr);
}
