#pragma once
#include <boot.h>

namespace mem {
    class PMM {
    public:
        // I should really create a separate struct
        // to hold the information I want to pass to PMM instead of boot_info_t
        // but I'll deal with it later
        PMM(struct boot_info*);
        
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
