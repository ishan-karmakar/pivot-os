#pragma once
#include <cpu/gdt.hpp>

namespace cpu {
    template <uint16_t L>
    class TSS {
    public:
        TSS(GDT<L>&);

    private:
        struct [[gnu::packed]] tss {
            uint32_t rsv0;
            uint64_t rsp0, rsp1, rsp2;
            uint64_t rsv1;
            uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
            uint64_t rsv2;
            uint16_t rsv3;
            uint16_t iopb;
        };

        GDT<L>& gdt;
    };
}