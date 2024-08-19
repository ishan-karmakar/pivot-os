#pragma once
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>

namespace scheduler {
    enum thread_lvl {
        Superuser,
        User
    };

    class process {
    public:
        process(const char*, thread_lvl);
    
    private:
        const char *name;
        mapper::ptmapper mapper;
        vmm::vmm vmm;
        // heap::policy policy;
        // heap::pool_t pool;
        // cpu::status ef;
    };
}