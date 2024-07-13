#pragma once
#include <cstddef>
#include <cstdint>

namespace mem {
    // This class is meant to be inherited by another class such as VMM, Heap
    class Bitmap {
    protected:
        void init();
        void *alloc(size_t);
        size_t free(void*);
        void *realloc(void*, size_t);

    private:
        typedef uint8_t id_t;

        void set_id(id_t, id_t);
        id_t get_id(size_t);
        id_t unique_id(id_t, id_t);
        size_t num_blocks();

        size_t tsize, bsize;
        size_t used;
        size_t ffa;
        uint8_t *bm;

        const uint8_t BITS_PER_ID = 2;
        const uint8_t BLOCKS_PER_INT = sizeof(id_t) * 8 / BITS_PER_ID;
    };
};