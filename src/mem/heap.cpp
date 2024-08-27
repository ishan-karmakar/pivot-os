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

allocator::allocator() : frg::slab_allocator<policy, frg::simple_spinlock>{heap_pool.get()} {}

void heap::init() {
    heap_slab_policy.initialize(*vmm::kvmm);
    heap_pool.initialize(*heap_slab_policy);
    logger::info("HEAP", "Initialized slab allocator");
}

void *malloc(std::size_t size) {
    return heap::allocator().allocate(size);
}

void *calloc(std::size_t count, std::size_t size) {
    size *= count;
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void *realloc(void *old, std::size_t size) {
    return heap::allocator().reallocate(old, size);
}

void free(void *ptr) {
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

extern "C" {
    [[gnu::alias("malloc"), gnu::malloc, gnu::alloc_size(1)]] void *term_alloc(std::size_t) noexcept;
    [[gnu::alias("realloc"), gnu::alloc_size(2)]] void *term_realloc(void*, std::size_t) noexcept;
    [[gnu::alias("free")]] void term_free(void*) noexcept;
    [[gnu::alias("free")]] void term_freensz(void*) noexcept;
}

[[gnu::alias("calloc"), gnu::malloc, gnu::alloc_size(1, 2)]] void *uacpi_kernel_calloc(std::size_t, std::size_t) noexcept;
[[gnu::alias("malloc"), gnu::malloc, gnu::alloc_size(1)]] void *uacpi_kernel_alloc(std::size_t) noexcept;
[[gnu::alias("free")]] void uacpi_kernel_free(void*) noexcept;
