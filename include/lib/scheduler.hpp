#pragma once
#include <frg/spinlock.hpp>
#include <lib/proc.hpp>
#include <queue>

namespace scheduler {
    extern frg::simple_spinlock ready_lock, wakeup_lock;
    extern std::queue<proc::process*> ready_proc;

    void init();
    void start();
    cpu::status *schedule(cpu::status*);
}