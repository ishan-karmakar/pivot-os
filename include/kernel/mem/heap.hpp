#pragma once
#include <mem/bitmap.hpp>

namespace mem {
    class Heap : public Bitmap {
    public:
        Heap(void*, size_t, size_t);
    };
}