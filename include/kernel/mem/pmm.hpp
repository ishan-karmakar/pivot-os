#pragma once
#include <boot.h>

namespace mem {
    class PMM {
    public:
        PMM() = delete;

        static void init(boot_info*);
        
        static uintptr_t frame();

        static void clear(uintptr_t, size_t);
        static void clear(uintptr_t);
        
        static void set(uintptr_t, size_t);
        static void set(uintptr_t);

    private:
        static size_t bitmap_size;
        static uint8_t *bitmap;
    };
}
