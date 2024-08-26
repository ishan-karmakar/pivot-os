#pragma once
#include <cstddef>
#include <cpu/tss.hpp>
#include <lib/scheduler.hpp>
struct limine_smp_info;

namespace smp {
    struct cpu_t {
        std::size_t id;
        volatile bool ready;
        tss::tss tss;
        scheduler::process *cur_proc;
        void *fpu_data;
        std::size_t lapic_ticks;
    };

    extern std::size_t cpu_count;
    extern cpu_t *cpus;

    void early_init();
    void init();
    void ap_init(limine_smp_info*);
    cpu_t *this_cpu();
}