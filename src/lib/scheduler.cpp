#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>
#include <lib/interrupts.hpp>
#include <drivers/pit.hpp>
#include <lib/timer.hpp>
#include <lib/syscall.hpp>
#include <cpu/smp.hpp>
#include <queue>
#include <frg/rbtree.hpp>
using namespace scheduler;

constexpr int PROC_QUANTUM = 100;
static std::size_t quantum;

struct proc_comparator {
    constexpr bool operator()(const process& l, const process& r) const { return l.wakeup < r.wakeup; }
};

std::queue<process*> ready_proc;
frg::simple_spinlock ready_lock;
frg::rbtree<process, &process::hook, proc_comparator> wakeup_proc;
// Essentially a symlink to the first process in wakeup_proc to speed up processing in schedule
process *first_wakeup;
frg::simple_spinlock wakeup_lock;
process *idle_proc;

cpu::status *schedule(cpu::status*);
void proc_wrapper(void (*)());

[[gnu::naked]]
void idle() { while(1) asm ("hlt"); }

void scheduler::init() {
    quantum = smp::cpu_count * PROC_QUANTUM;
    for (std::size_t i = 0; i < smp::cpu_count; i++)
        smp::cpus[i].sched_off = i * PROC_QUANTUM;
    idle_proc = new process{"idle", idle, true, *vmm::kvmm, *heap::policy, *heap::pool, PAGE_SIZE};
    logger::info("SCHED", "Initialized scheduler");
}

void scheduler::start() {
    logger::info("SCHED", "Starting scheduler");
    if (lapic::initialized)
        idt::handlers[timer::irq].push_back(schedule);
    else
        idt::handlers[timer::irq].push_back(schedule);
}

process::process(const char *name, void (*addr)(), bool superuser, std::size_t stack_size) :
process{
    name,
    addr,
    superuser,
    *({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        for (int i = 256; i < 512; i++) tbl[i] = mapper::kmapper->data()[i];
        auto m = new mapper::ptmapper{tbl};
        m->load();
        new vmm::vmm{PAGE_SIZE, stack_size + heap::policy_t::slabsize, superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, *m};
    }),
    *new heap::policy_t{vmm},
    *new heap::pool_t{policy},
    stack_size
} {}

process::process(const char *name, void (*addr)(), bool superuser, vmm::vmm& vmm, heap::policy_t& policy, heap::pool_t &pool, std::size_t stack_size) :
    name{name},
    vmm{vmm},
    policy{policy},
    pool{pool},
    fpu_data{operator new(cpu::fpu_size)}
{
    ef.rsp = reinterpret_cast<uintptr_t>(vmm.malloc(stack_size)) + stack_size;
    ef.rip = reinterpret_cast<uintptr_t>(proc_wrapper);
    ef.rdi = reinterpret_cast<uintptr_t>(addr);
    ef.rflags = 0x202;
    if (superuser) {
        ef.cs = gdt::KCODE;
        ef.ss = gdt::KDATA;
    } else {
        ef.cs = gdt::UCODE | 3;
        ef.ss = gdt::UDATA | 3;
    }

    mapper::kmapper->load();
}

void process::enqueue() {
    ready_lock.lock();
    ready_proc.push(this);
    ready_lock.unlock();
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
            memcpy(cur_proc->fpu_data, smp::this_cpu()->fpu_data, cpu::fpu_size);
            if (cur_proc->status == Ready)
                ready_proc.push(cur_proc);
            else if (cur_proc->status == Sleep) {
                wakeup_proc.insert(cur_proc);
                first_wakeup = wakeup_proc.first();
            }
        }
    }
}

cpu::status *schedule(cpu::status *status) {
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
    memcpy(smp::this_cpu()->fpu_data, cur_proc->fpu_data, cpu::fpu_size);
    return &cur_proc->ef;
}

void proc_wrapper(void (*fn)()) {
    fn();
    syscall(SYS_exit);
}

cpu::status *scheduler::sys_exit(cpu::status *status) {
    smp::this_cpu()->cur_proc->status = Delete;
    return schedule(status);
}

cpu::status *scheduler::sys_nanosleep(cpu::status *status) {
    process*& cur_proc = smp::this_cpu()->cur_proc;
    cur_proc->status = Sleep;
    cur_proc->wakeup = timer::time() + (status->rsi / 1'000'000);
    return schedule(status);
}

void proc::sleep(std::size_t ms) {
    syscall(SYS_nanosleep, ms * 1'000'000);
}
