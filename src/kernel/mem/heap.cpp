#include <mem/heap.hpp>
using namespace mem;

// Heap is basically just a clone of bitmap
// since nothing special is needed to add to it
// All memory management is already taken care of by VMM
Heap::Heap(void *start, size_t size, size_t bsize) : Bitmap{size, bsize, static_cast<uint8_t*>(start)} {
    Bitmap::init();
}
