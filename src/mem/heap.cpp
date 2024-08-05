#include <mem/heap.hpp>
#include <util/logger.hpp>
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

uintptr_t HeapSlabPolicy::map(size_t size) {
    return reinterpret_cast<uintptr_t>(vmm::kvmm->malloc(round_up(size, PAGE_SIZE)));;
}

void HeapSlabPolicy::unmap(uintptr_t, size_t) {
    // TODO: Implement free
};

void heap::init() {
    heap_pool.initialize(heap_slab_policy);
    log(INFO, "HEAP", "Initialized kernel heap slab allocator");
}

HeapAllocator& heap::allocator() {
    static HeapAllocator alloc{heap_pool.get()};
    return alloc;
}

void *malloc(size_t size) {
    return heap::allocator().allocate(size);
}

void *calloc(size_t size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void *realloc(void *old, size_t size) {
    return heap::allocator().reallocate(old, size);
}

void free(void *ptr) {
    return heap::allocator().free(ptr);
}

void *operator new(size_t size) {
    return malloc(size);
}

void operator delete(void *ptr) {
    return free(ptr);
}

void operator delete(void *ptr, size_t) {
    return operator delete(ptr);
}

void *operator new[](size_t size) {
    return operator new(size);
}

void operator delete[](void *ptr) {
    return operator delete(ptr);
}

void operator delete[](void *ptr, size_t) {
    return operator delete[](ptr);
}

void *uacpi_kernel_alloc(size_t size) {
    return malloc(size);
}

void *uacpi_kernel_calloc(size_t count, size_t size) {
    return calloc(count * size);
}

void uacpi_kernel_free(void *ptr) {
    return free(ptr);
}

uacpi_handle uacpi_kernel_create_mutex() {
    return new std::atomic_flag;
}

// FIXME: Take into account timeout
bool uacpi_kernel_acquire_mutex(void* m, uint16_t) {
    std::atomic_flag *lock = static_cast<std::atomic_flag*>(m);
    while (lock->test_and_set(std::memory_order_acquire)) asm ("pause");
    return true;
}

void uacpi_kernel_release_mutex(void *m) {
    std::atomic_flag *lock = static_cast<std::atomic_flag*>(m);
    lock->clear();
}

void uacpi_kernel_free_mutex(void *mutex) {
    delete static_cast<std::atomic_flag*>(mutex);
}

uacpi_handle uacpi_kernel_create_spinlock() {
    // Right now, we treat mutexes and spinlocks the same
    log(INFO, "uACPI", "uACPI requested to create spinlock");
    return uacpi_kernel_create_mutex();
}

uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle) {
    log(INFO, "uACPI", "uACPI requested to lock spinlock");
    return 0;
}

void uacpi_kernel_spinlock_unlock(uacpi_handle, uacpi_cpu_flags) {
    log(INFO, "uACPI", "uACPI requested to unlock spinlock");
}

void uacpi_kernel_free_spinlock(uacpi_handle) {
    log(INFO, "uACPI", "uACPI requested to free spinlock");
}

uacpi_handle uacpi_kernel_create_event() {
    log(INFO, "uACPI", "uACPI requested to create event");
    return new std::atomic_uint64_t;
}

void uacpi_kernel_free_event(uacpi_handle e) {
    log(INFO, "uACPI", "uACPI requested to free event");
    delete static_cast<std::atomic_uint64_t*>(e);
}
