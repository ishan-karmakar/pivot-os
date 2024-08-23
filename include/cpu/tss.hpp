#pragma once
#include <cstdint>

namespace tss {
    void init();
    void set_rsp0(uintptr_t);

    [[gnu::always_inline]]
    inline void set_rsp0() {
        uintptr_t rsp;
        asm volatile ("mov %%rsp, %0" : "=r" (rsp));
        set_rsp0(rsp);
    }
}