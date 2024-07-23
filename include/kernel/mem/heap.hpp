#pragma once
#include <mem/bitmap.hpp>
// 64 KiB for kernel heap
#define HEAP_SIZE 16

namespace mem {
    class VMM;
    class Heap : public Bitmap {
    public:
        Heap(VMM&, size_t, size_t = 16);
    };

    extern Heap *kheap;
}