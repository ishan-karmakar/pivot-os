#pragma once
#include <boot.h>

namespace mem {
    class PhysicalMemoryManager {
    public:
        void init(struct boot_info*);
        
        uintptr_t frame();

        void clear(uintptr_t, size_t);
        void clear(uintptr_t);
        
        void set(uintptr_t, size_t);
        void set(uintptr_t);

    private:
        size_t bitmap_size;
        uint8_t *bitmap;
    };
}
