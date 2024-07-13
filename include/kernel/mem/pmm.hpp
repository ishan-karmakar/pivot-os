#pragma once
#include <boot.h>

namespace mem {
    class PhysicalMemoryManager {
    public:
        void init(struct boot_info*);
        void clear_region(uintptr_t, size_t);
        void clear_region(uintptr_t);
        void set_region(uintptr_t, size_t);
        void set_region(uintptr_t);

    private:
        size_t bitmap_size;
        uint8_t *bitmap;
    };
}
