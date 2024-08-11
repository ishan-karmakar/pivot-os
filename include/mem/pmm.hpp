#pragma once
#include <cstddef>
#include <cstdint>
#include <limine.h>

namespace pmm {
    extern size_t num_pages;
    
    void init();
    
    uintptr_t frame();

    void clear(uintptr_t, size_t);
    void clear(uintptr_t);
    
    void set(uintptr_t, size_t);
    void set(uintptr_t);
}

template <typename T>
T virt_addr(const T& phys) {
    extern volatile limine_hhdm_request hhdm_request;
    uintptr_t p = reinterpret_cast<uintptr_t>(phys);
    if (p >= hhdm_request.response->offset) return phys;
    return reinterpret_cast<T>(p + hhdm_request.response->offset);
}

template <typename T>
T phys_addr(const T& virt) {
    extern volatile limine_hhdm_request hhdm_request;
    uintptr_t v = reinterpret_cast<uintptr_t>(virt);
    if (v < hhdm_request.response->offset) return virt;
    return reinterpret_cast<T>(v - hhdm_request.response->offset);
}
