#pragma once
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>
#include <frg/manual_box.hpp>

namespace scheduler {
    enum thread_lvl {
        Superuser,
        User
    };

    enum status {
        New,
        Ready,
        Sleeping
    };

    class process {
    public:
        process(const char*, thread_lvl);
    
    private:
        const char *name;
        scheduler::status status{New};
        mapper::ptmapper mapper;
        vmm::vmm vmm;
        heap::policy policy;
        heap::pool_t pool;
        cpu::status ef;
    };
}