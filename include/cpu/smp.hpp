#pragma once
#include <cstddef>
#include <cpu/tss.hpp>
#include <lib/proc.hpp>
struct limine_smp_info;

namespace smp {
    struct cpu_t {
        std::size_t id;
        volatile bool ready;
        tss::tss tss;
        proc::process *cur_proc;
        std::size_t lapic_ticks;
        std::size_t sched_off;
        bool cpu_proc;
    };

    extern std::size_t cpu_count;
    extern cpu_t *cpus;
    extern std::size_t bsp_id;

    void early_init();
    void init();
    void ap_init(limine_smp_info*);
    cpu_t *this_cpu();
}