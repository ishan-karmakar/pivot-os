#pragma once
#include <mem/bitmap.hpp>
#include <frg/manual_box.hpp>
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

    namespace heap {
        void init();
    }

    extern frg::manual_box<Heap> kheap;
}