#pragma once
#include <mem/bitmap.hpp>
#include <frg/manual_box.hpp>
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <kernel.hpp>
// 64 KiB for kernel heap
#define HEAP_SIZE 16

namespace mem {
    class VMM;
    class Heap : public Bitmap {
    public:
        Heap(VMM&, size_t, size_t = 16);
    };

    struct HeapSlabPolicy {
        uintptr_t map(size_t);
        void unmap(uintptr_t);
    };
    typedef frg::slab_allocator<HeapSlabPolicy, frg::simple_spinlock> HeapAllocator;

    namespace heap {
        void init();
    }

    extern frg::manual_box<Heap> kheap;
}