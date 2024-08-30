#pragma once
#include <frg/spinlock.hpp>
#include <lib/proc.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>
#include <queue>

namespace scheduler {
    extern frg::simple_spinlock ready_lock, wakeup_lock;
    extern frg::hash_map<int64_t, std::queue<proc::process*>, frg::hash<int64_t>, heap::allocator> ready_proc;

    void init();
    void start();
    cpu::status *schedule(cpu::status*);
}