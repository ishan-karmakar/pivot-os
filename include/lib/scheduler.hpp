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
        Sleep,
        Delete
    };

    void start();
    cpu::status *schedule(cpu::status*);

    class process {
    public:
        friend cpu::status *schedule(cpu::status*);

        process(const char*, uintptr_t, bool);
        void enqueue();
        process *prev;
        process *next;
    
    private:
        const char *name;
        std::size_t wakeup;
        scheduler::status status{New};
        mapper::ptmapper mapper;
        vmm::vmm vmm;
        heap::policy policy;
        heap::pool_t pool;
        cpu::status ef;
    };

}