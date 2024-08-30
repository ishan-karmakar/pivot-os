#include <lib/scheduler.hpp>
#include <lib/proc.hpp>
#include <lib/logger.hpp>
#include <lib/timer.hpp>
#include <cpu/smp.hpp>
#include <cpu/idt.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <drivers/lapic.hpp>
#include <queue>
using namespace scheduler;
using namespace proc;

constexpr int PROC_QUANTUM = 100;
static std::size_t quantum;

struct proc_comparator {
    constexpr bool operator()(const process& l, const process& r) const { return l.wakeup < r.wakeup; }
};

std::queue<process*> scheduler::ready_proc;
frg::simple_spinlock scheduler::ready_lock, scheduler::wakeup_lock;
frg::rbtree<process, &process::hook, proc_comparator> wakeup_proc;
// Essentially a symlink to the first process in wakeup_proc to speed up processing in schedule
process *first_wakeup;
process *idle_proc;

cpu::status *schedule(cpu::status*);

[[gnu::naked]]
void idle() { while(1) asm ("hlt"); }

void scheduler::init() {
    quantum = smp::cpu_count * PROC_QUANTUM;
    for (std::size_t i = 0; i < smp::cpu_count; i++)
        smp::cpus[i].sched_off = i * PROC_QUANTUM;
    idle_proc = new process{idle, true, *vmm::kvmm, *heap::pool, PAGE_SIZE};
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
                ready_proc.push(cur_proc);
            else if (cur_proc->status == Sleep) {
                wakeup_proc.insert(cur_proc);
                first_wakeup = wakeup_proc.first();
            }
        }
    }
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
    bool proc_avail = !ready_proc.empty() || (first_wakeup && timer::time() >= first_wakeup->wakeup);
    if (!(
        (cur_proc && (cur_proc->status == Delete || cur_proc->status == Sleep)) ||
        (cur_proc == idle_proc && proc_avail) ||
        (!((timer::time() + smp::this_cpu()->sched_off) % quantum) && proc_avail)
    )) return status;
    {
        ready_lock.lock();
        wakeup_lock.lock();
        save_proc(cur_proc, status);
        if (first_wakeup && timer::time() >= first_wakeup->wakeup) {
            first_wakeup->status = Ready;
            wakeup_proc.remove(first_wakeup);
            cur_proc = first_wakeup;
            first_wakeup = wakeup_proc.first();
        } else if (!ready_proc.empty()) {
            cur_proc = ready_proc.front();
            ready_proc.pop();
            cur_proc->status = Ready;
        } else
            cur_proc = idle_proc;
        wakeup_lock.unlock();
        ready_lock.unlock();
    }

    auto cr3 = phys_addr(reinterpret_cast<uintptr_t>(cur_proc->vmm.mapper().data()));
    cur_proc->ef.cr3 = rdreg(cr3) == cr3 ? 0 : cr3;
    if (cpu::fpu_restore) cpu::fpu_restore(cur_proc->fpu_data);
    return &cur_proc->ef;
}
