#include <mem/heap.hpp>
#include <util/logger.h>
#include <libc/string.h>
using namespace mem;

// Heap is basically just a clone of bitmap since nothing special is needed to add to it
// All memory management is already taken care of by VMM
Heap::Heap(VMM& vmm, size_t size, size_t bsize) :
    Bitmap{size, bsize, reinterpret_cast<uint8_t*>(vmm.alloc(size))}
{
    log(Info, "HEAP", "Initialized heap");
}

void *Heap::calloc(size_t size) {
    void *ptr = alloc(size);
    memset(ptr, 0, size);
    return ptr;
}
