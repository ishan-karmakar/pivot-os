#pragma once
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>
#include <frg/manual_box.hpp>

namespace scheduler {
    enum status {
        New,
        Ready,
        Sleeping
    };

    void start();

    class process {
    public:
        process(const char*, uintptr_t, bool);
        void enqueue();
    
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