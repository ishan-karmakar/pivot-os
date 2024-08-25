#pragma once
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <mem/heap.hpp>
#include <frg/manual_box.hpp>

namespace scheduler {
    constexpr std::size_t THREAD_STACK = 2 * PAGE_SIZE;
    constexpr std::size_t THREAD_HEAP = heap::policy::sb_size;

    enum status {
        New,
        Ready,
        Sleep,
        Delete
    };

    void init();
    void start();

    struct process {
        process(const char*, void (*)(), bool, std::size_t = THREAD_STACK, std::size_t = THREAD_HEAP);
        void enqueue();
    
        const char *name;
        std::size_t wakeup;
        scheduler::status status{New};
        mapper::ptmapper mapper;
        vmm::vmm vmm;
        heap::policy policy;
        heap::pool_t pool;
        cpu::status ef;
        void *fpu_data;
    };

    cpu::status *sys_exit(cpu::status*);
    cpu::status *sys_nanosleep(cpu::status*);
}

namespace proc {
    // Sleep for N ms
    void sleep(std::size_t);
};