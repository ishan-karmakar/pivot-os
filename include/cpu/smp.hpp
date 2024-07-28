#pragma once
#include <cstddef>

namespace cpu::smp {
    extern size_t num_cpus;

    void init_bsp();
    void init_ap();
}