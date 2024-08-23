#pragma once
#include <cstddef>
#include <cpu/cpu.hpp>
struct limine_smp_info;

namespace smp {
    struct cpu_t {
        std::size_t id;
        volatile bool ready;
    };

    void init();
    void ap_init(limine_smp_info*);

    inline std::size_t cpu_id() {
        auto gs = cpu::get_kgs();
        return gs == 0 ? 0 : reinterpret_cast<cpu_t*>(gs)->id;
    }
}