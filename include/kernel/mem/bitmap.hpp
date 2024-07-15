#pragma once
#include <cstddef>
#include <cstdint>

namespace mem {
    // This class is meant to be inherited by another class such as VMM, Heap
    class Bitmap {
    public:
        virtual void *alloc(size_t);
        virtual size_t free(void*);
        void *realloc(void*, size_t);

    protected:
        // Allows only subclasses to call it. Only initializes values, doesn't do any operations
        Bitmap(size_t, size_t, uint8_t*);

        // Does initialization and accesses memory
        // Constructor and init() are split so that subclass can choose to setup memory for init since base constructor must run before subclass constructor
        // Ex: VMM maps the area that BM init will access
        void init();
    
    private:
        void set_id(size_t, uint8_t);
        uint8_t get_id(size_t);
        uint8_t unique_id(uint8_t, uint8_t);

        size_t tsize, bsize;
        uint8_t *bm;
        size_t hblocks;
        size_t used;
        size_t ffa;
    };
};