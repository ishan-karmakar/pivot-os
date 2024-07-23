#include <mem/heap.hpp>
#include <util/logger.h>
#include <libc/string.h>
#include <mem/vmm.hpp>
#include <uacpi/kernel_api.h>
#include <atomic>
#include <cstdlib>
#include <common.h>

using namespace mem;

mem::Heap *mem::kheap = nullptr;

// Heap is basically just a clone of bitmap since nothing special is needed to add to it
// All memory management is already taken care of by VMM
Heap::Heap(VMM& vmm, size_t size, size_t bsize) :
    Bitmap{size * PAGE_SIZE, bsize, reinterpret_cast<uint8_t*>(vmm.malloc(size))}
{
    log(Info, "HEAP", "Initialized heap");
}

void *malloc(size_t size) {
    if (kheap)
        return kheap->malloc(size);
    log(Error, "HEAP", "malloc called before heap was initialized");
    abort();
}

void *calloc(size_t size) {
    if (kheap)
        return kheap->calloc(size);
    log(Error, "HEAP", "calloc called before heap was initialized");
    abort();
}

void *realloc(void *old, size_t size) {
    if (kheap)
        return kheap->realloc(old, size);
    log(Error, "HEAP", "realloc called before heap was initialized");
    abort();
}

void free(void *ptr) {
    if (kheap) {
        kheap->free(ptr);
        return;
    }
    log(Error, "HEAP", "free called before heap was initialized");
    abort();
}

void *operator new(size_t size) {
    return malloc(size);
}

void operator delete(void *ptr) {
    return free(ptr);
}

void *operator new[](size_t size) {
    return operator new(size);
}

void operator delete[](void *ptr) {
    return operator delete(ptr);
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

// FIXME: Optimize this; every time we create a mutex we waste 15 bytes of memory
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
    log(Info, "uACPI", "uACPI requested to create spinlock");
    return uacpi_kernel_create_mutex();
}

uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle m) {
    log(Info, "uACPI", "uACPI requested to lock spinlock");
}

void uacpi_kernel_spinlock_unlock(uacpi_handle, uacpi_cpu_flags) {
    log(Info, "uACPI", "uACPI requested to unlock spinlock");
}

void uacpi_kernel_free_spinlock(uacpi_handle) {
    log(Info, "uACPI", "uACPI requested to free spinlock");
}

uacpi_handle uacpi_kernel_create_event() {
    log(Info, "uACPI", "uACPI requested to create event");
    return new std::atomic_uint64_t;
}

void uacpi_kernel_free_event(uacpi_handle e) {
    log(Info, "uACPI", "uACPI requested to free event");
    delete static_cast<std::atomic_uint64_t*>(e);
}
