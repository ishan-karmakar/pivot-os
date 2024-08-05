#pragma once
#include <cstddef>
#include <cstdint>

namespace pmm {
    extern size_t num_pages;
    
    void init();
    
    uintptr_t frame();

    void clear(uintptr_t, size_t);
    void clear(uintptr_t);
    
    void set(uintptr_t, size_t);
    void set(uintptr_t);
}

uintptr_t virt_addr(uintptr_t);
uintptr_t phys_addr(uintptr_t);
