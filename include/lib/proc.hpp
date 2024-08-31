#pragma once
#include <frg/rbtree.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>
#include <functional>

namespace vmm {
    class vmm;
}

namespace proc {
    constexpr std::size_t PROC_STACK = 2 * PAGE_SIZE;

    enum status {
        New,
        Ready,
        Sleep,
        Delete
    };

    struct process {
        process(uintptr_t addr, bool, std::size_t = PROC_STACK);
        process(uintptr_t addr, bool, vmm::vmm&, heap::pool_t&, std::size_t = PROC_STACK);
        ~process();
        void enqueue();
    
        std::size_t pid;
        int64_t cpu;
        bool auto_create; // True if VMM and pool where automatically created
        frg::rbtree_hook hook;
        std::size_t wakeup;
        proc::status status{New};
        cpu::status ef;
        vmm::vmm& vmm;
        heap::pool_t& pool;
        void *fpu_data;
    };

    cpu::status *sys_exit(cpu::status*);
    cpu::status *sys_nanosleep(cpu::status*);
    cpu::status *sys_getpid(cpu::status*);
}