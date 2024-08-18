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

constinit HeapSlabPolicy heap_slab_policy;
frg::manual_box<frg::slab_pool<HeapSlabPolicy, frg::simple_spinlock>> heap_pool;

uintptr_t HeapSlabPolicy::map(std::size_t size) {
    return reinterpret_cast<uintptr_t>(vmm::kvmm->malloc(round_up(size, PAGE_SIZE)));;
}

void HeapSlabPolicy::unmap(uintptr_t addr, std::size_t) {
    vmm::kvmm->free(reinterpret_cast<void*>(addr));
};

void heap::init() {
    heap_pool.initialize(heap_slab_policy);
    logger::info("HEAP[INIT]", "Initialized slab allocator");
}

HeapAllocator& heap::allocator() {
    static HeapAllocator alloc{heap_pool.get()};
    return alloc;
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
