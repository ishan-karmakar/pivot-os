#pragma once
#include <boot.h>

namespace mem {
    namespace pmm {
        void init(boot_info*);
        
        uintptr_t frame();

        void clear(uintptr_t, size_t);
        void clear(uintptr_t);
        
        void set(uintptr_t, size_t);
        void set(uintptr_t);
    };
}
