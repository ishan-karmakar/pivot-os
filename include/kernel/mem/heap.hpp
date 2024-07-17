#pragma once
#include <mem/bitmap.hpp>
#include <mem/vmm.hpp>
// 16 KiB for kernel heap
#define HEAP_SIZE (PAGE_SIZE * 4)

namespace mem {
    class Heap : public Bitmap {
    public:
        Heap(VMM&, size_t, size_t = 16);
        void *calloc(size_t);
    };

    extern Heap *kheap;
}