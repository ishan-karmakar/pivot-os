#include <lib/scheduler.hpp>
#include <lib/proc.hpp>
#include <lib/logger.hpp>
#include <lib/timer.hpp>
#include <cpu/smp.hpp>
#include <cpu/idt.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <drivers/lapic.hpp>
#include <frg/hash_map.hpp>
#include <queue>
using namespace scheduler;
using namespace proc;

constexpr int PROC_QUANTUM = 100;
static std::size_t quantum;

struct proc_comparator {
    constexpr bool operator()(const process& l, const process& r) const { return l.wakeup < r.wakeup; }
};

frg::simple_spinlock scheduler::ready_lock, scheduler::wakeup_lock;
frg::hash_map<int64_t, std::queue<process*>, frg::hash<int64_t>, heap::allocator> scheduler::ready_proc{{}};
frg::hash_map<int64_t, std::pair<process*, frg::rbtree<process, &process::hook, proc_comparator>*>, frg::hash<int64_t>, heap::allocator> wakeup_proc{{}};
// Essentially a symlink to the first process in wakeup_proc to speed up processing in schedule
process *idle_proc;

cpu::status *schedule(cpu::status*);

void idle() { while(1) asm ("hlt"); }

void scheduler::init() {
    quantum = smp::cpu_count * PROC_QUANTUM;
    for (std::size_t i = 0; i < smp::cpu_count; i++)
        smp::cpus[i].sched_off = i * PROC_QUANTUM;
    idle_proc = new process{reinterpret_cast<uintptr_t>(idle), true, *vmm::kvmm, *heap::pool, PAGE_SIZE};
    logger::info("SCHED", "Initialized scheduler");
}

void scheduler::start() {
    logger::info("SCHED", "Starting scheduler");
    if (lapic::initialized)
        idt::handlers[timer::irq].push_back(schedule);
    else
        idt::handlers[timer::irq].push_back(schedule);
}

static void save_proc(process*& cur_proc, cpu::status *status) {
    if (cur_proc && cur_proc != idle_proc) {
        if (cur_proc->status == Delete)
            delete cur_proc;
        else if (cur_proc->status == Ready || cur_proc->status == Sleep) {
            // This is really dirty but it's to skip the cr3 at the beginning of status
            memcpy(
                reinterpret_cast<uintptr_t*>(&cur_proc->ef) + 1,
                reinterpret_cast<uintptr_t*>(status) + 1,
                sizeof(cpu::status) - sizeof(uintptr_t)
            );
            if (cpu::fpu_save) cpu::fpu_save(cur_proc->fpu_data);
            if (cur_proc->status == Ready)
                ready_proc[cur_proc->cpu].push(cur_proc);
            else if (cur_proc->status == Sleep) {
                auto& e = wakeup_proc[cur_proc->cpu];
                if (e.second == nullptr)
                    e.second = new frg::rbtree<process, &process::hook, proc_comparator>{};
                e.second->insert(cur_proc);
                e.first = e.second->first();
            }
        }
    }
}

static void handle_wakeup(process*& cur_proc, int64_t id) {
    cur_proc = wakeup_proc[id].first;
    wakeup_proc[id].second->remove(cur_proc);
    wakeup_proc[id].first = wakeup_proc[id].second->first();
}

static void handle_ready(process*& cur_proc, int64_t id) {
    cur_proc = ready_proc[id].front();
    ready_proc[id].pop();
}

cpu::status *scheduler::schedule(cpu::status *status) {
    process*& cur_proc = smp::this_cpu()->cur_proc;
    while (wakeup_lock.is_locked() && ready_lock.is_locked()) asm ("pause");
    /*
    Change thread if any of the following occurs:
    - A sleeping thread wakes up now
    - The current thread is sleeping or is marked for deletion
    - The current thread is the idle thread and there is a thread ready to run
    - The thread's quantum is up
    */
    // bool proc_avail = ready_sizes[-1] || ready_sizes[smp::this_cpu()->id] || (first_wakeup && timer::time() >= first_wakeup->wakeup);
    auto t = timer::time();
    auto id = smp::this_cpu()->id;
    bool wakeup_cpu_specific = wakeup_proc[id].first && t >= wakeup_proc[id].first->wakeup;
    bool wakeup_generic = wakeup_proc[-1].first && t >= wakeup_proc[-1].first->wakeup;
    bool ready_cpu_specific = ready_proc[id].size();
    bool ready_generic = ready_proc[-1].size();
    bool proc_avail = wakeup_generic || wakeup_cpu_specific || ready_generic || ready_cpu_specific;
    if (!(
        (cur_proc && (cur_proc->status == Delete || cur_proc->status == Sleep)) ||
        (cur_proc == idle_proc && proc_avail) ||
        (!((t + smp::this_cpu()->sched_off) % quantum) && proc_avail)
    )) return status;
    ready_lock.lock();
    wakeup_lock.lock();
    save_proc(cur_proc, status);
    if (smp::this_cpu()->cpu_proc) {
        if (wakeup_cpu_specific) {
            handle_wakeup(cur_proc, id);
            goto finish;
        }
        if (wakeup_generic) {
            handle_wakeup(cur_proc, -1);
            goto finish;
        }
        if (ready_cpu_specific) {
            handle_ready(cur_proc, id);
            goto finish;
        }
        if (ready_generic) {
            handle_ready(cur_proc, -1);
            goto finish;
        }
    } else {
        if (wakeup_generic) {
            handle_wakeup(cur_proc, -1);
            goto finish;
        }
        if (wakeup_cpu_specific) {
            handle_wakeup(cur_proc, id);
            goto finish;
        }
        if (ready_generic) {
            handle_ready(cur_proc, -1);
            goto finish;
        }
        if (ready_cpu_specific) {
            handle_ready(cur_proc, id);
            goto finish;
        }
    }
    cur_proc = idle_proc;

finish:
    wakeup_lock.unlock();
    ready_lock.unlock();
    smp::this_cpu()->cpu_proc ^= true;

    cur_proc->status = Ready;
    auto cr3 = phys_addr(reinterpret_cast<uintptr_t>(cur_proc->vmm.mapper().data()));
    cur_proc->ef.cr3 = rdreg(cr3) == cr3 ? 0 : cr3;
    if (cpu::fpu_restore) cpu::fpu_restore(cur_proc->fpu_data);
    return &cur_proc->ef;
}
