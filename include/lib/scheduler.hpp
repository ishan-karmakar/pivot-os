#pragma once
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>
#include <frg/rbtree.hpp>

namespace scheduler {
    constexpr std::size_t PROC_STACK = 2 * PAGE_SIZE;

    enum status {
        New,
        Ready,
        Sleep,
        Delete
    };

    void init();
    void start();

    struct process {
        process(const char*, void (*)(), bool, std::size_t = PROC_STACK);
        process(const char*, void (*)(), bool, vmm::vmm&, heap::pool_t&, std::size_t = PROC_STACK);
        void enqueue();
    
        const char *name;
        frg::rbtree_hook hook;
        std::size_t wakeup;
        scheduler::status status{New};
        cpu::status ef;
        vmm::vmm& vmm;
        heap::pool_t& pool;
        void *fpu_data;
    };

    cpu::status *sys_exit(cpu::status*);
    cpu::status *sys_nanosleep(cpu::status*);
}

namespace proc {
    // Sleep for N ms
    void sleep(std::size_t);
};