#pragma once
#include <mem/bitmap.hpp>
#include <mem/vmm.hpp>

namespace mem {
    class Heap : public Bitmap {
    public:
        Heap(VMM&, size_t, size_t = 16);
    };
}