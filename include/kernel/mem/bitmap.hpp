#pragma once
#include <cstddef>
#include <cstdint>

namespace mem {
    // This class is meant to be inherited by another class such as VMM, Heap
    class Bitmap {
    public:
        void *alloc(size_t);
        void free(void*);
        void *realloc(void*, size_t);

    // protected:
        void init(size_t, size_t, uint8_t*);

        // Runs after alloc()
        // Meant for something like VMM mapping pages
        // This way it still works in correct order when using realloc
        virtual void post_alloc(void*, size_t) = 0;

        // Runs after free()
        // Meant for something like VMM freeing pages
        // This way it still works in correct order when using realloc
        virtual void post_free(void*, size_t) = 0;

    private:
        void set_id(size_t, uint8_t);
        uint8_t get_id(size_t);
        uint8_t unique_id(uint8_t, uint8_t);
        size_t header_blocks();

        size_t tsize, bsize;
        size_t used;
        size_t ffa;
        uint8_t *bm;
    };
};