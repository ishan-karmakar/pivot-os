#pragma once
#include <mem/bitmap.hpp>
#include <mem/vmm.hpp>
#include <common.h>
// 8 KiB for kernel heap
#define HEAP_SIZE (PAGE_SIZE * 2)

namespace mem {
    class Heap : public Bitmap {
    public:
        Heap(VMM&, size_t, size_t = 16);
        void *calloc(size_t);
    };

    extern Heap *kheap;
}