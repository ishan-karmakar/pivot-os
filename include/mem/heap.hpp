#pragma once
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <kernel.hpp>

namespace heap {
    struct HeapSlabPolicy {
        uintptr_t map(std::size_t);
        void unmap(uintptr_t, std::size_t);
    };

    typedef frg::slab_allocator<HeapSlabPolicy, frg::simple_spinlock> HeapAllocator;

    void init();
    HeapAllocator& allocator();
}