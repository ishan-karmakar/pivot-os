#pragma once
#include <cstdint>

namespace tss {
    struct [[gnu::packed]] tss_t {
        uint32_t rsv0;
        uint64_t rsp0, rsp1, rsp2;
        uint64_t rsv1;
        uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
        uint64_t rsv2;
        uint16_t rsv3;
        uint16_t iopb;
    };

    void init();
    void set_rsp0(uintptr_t);

    [[gnu::always_inline]]
    inline void set_rsp0() {
        uintptr_t rsp;
        asm volatile ("mov %%rsp, %0" : "=r" (rsp));
        set_rsp0(rsp);
    }
}