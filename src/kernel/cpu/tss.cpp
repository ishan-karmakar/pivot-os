#include <cpu/tss.hpp>
using namespace cpu;

template <uint16_t L>
TSS<L>::TSS(GDT<L>& gdt) {
    uint16_t idx = gdt.ff_idx();
}