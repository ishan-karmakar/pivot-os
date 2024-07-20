#pragma once
#include <cstddef>
#include <cstdint>

namespace mem {
    // This class is meant to be inherited by another class such as VMM, Heap
    class Bitmap {
    public:
        virtual void *malloc(size_t);
        virtual size_t free(void*);
        void *realloc(void*, size_t);
        void *calloc(size_t);

    protected:
        // Allows only subclasses to call it
        Bitmap(size_t, size_t, uint8_t*);

    private:
        void set_id(size_t, uint8_t);
        uint8_t get_id(size_t) const;
        uint8_t unique_id(uint8_t, uint8_t) const;

        size_t tsize, bsize;
        uint8_t *bm;
        size_t hblocks;
        size_t used;
        size_t ffa;
    };
};