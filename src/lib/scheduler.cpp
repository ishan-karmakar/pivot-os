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

constexpr int THREAD_QUANTUM = 100;

struct proc_comparator {
    constexpr bool operator()(const process& l, const process& r) const { return l.wakeup < r.wakeup; }
};

std::queue<process*> ready_proc;
frg::simple_spinlock ready_lock;
frg::rbtree<process, &process::hook, proc_comparator> wakeup_proc;
frg::simple_spinlock wakeup_lock;
process *idle_proc;

cpu::status *schedule(cpu::status*);
void proc_wrapper(void (*)());

[[gnu::naked]]
void idle() { while(1) asm ("pause"); }

void scheduler::init() {
    idle_proc = new process{"idle", idle, true, PAGE_SIZE, 0};
}

void scheduler::start() {
    logger::info("SCHED", "Starting scheduler");
    for (std::size_t i = 0; i < smp::cpu_count; i++)
        smp::cpus[i].sched_off = i * THREAD_QUANTUM / smp::cpu_count;

    if (lapic::initialized)
        idt::handlers[timer::irq].push_back(schedule);
    else
        idt::handlers[timer::irq].push_back(schedule);
}

process::process(const char *name, void (*addr)(), bool superuser, std::size_t stack_size, std::size_t heap_size) :
    name{name},
    mapper{({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        for (int i = 256; i < 512; i++) tbl[i] = mapper::kmapper->data()[i];
        mapper::ptmapper m{tbl};
        m.load();
        std::move(m);
    })},
    vmm{PAGE_SIZE, stack_size + heap_size, superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, mapper},
    policy{vmm},
    pool{policy},
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
        ef.cs = gdt::UCODE;
        ef.ss = gdt::UDATA;
    }

    mapper::kmapper->load(); // Reset back to kernel's address space
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
            memcpy(&cur_proc->ef, status, sizeof(cpu::status));
            memcpy(cur_proc->fpu_data, smp::this_cpu()->fpu_data, cpu::fpu_size);
            if (cur_proc->status == Ready)
                ready_proc.push(cur_proc);
            else if (cur_proc->status == Sleep)
                wakeup_proc.insert(cur_proc);
        }
    }
}

cpu::status *schedule(cpu::status *status) {
    process*& cur_proc = smp::this_cpu()->cur_proc;
    // Change process is any 
    wakeup_lock.lock();
    if (wakeup_proc.get_root()) {
        auto proc = wakeup_proc.first();
        if (timer::time() >= proc->wakeup) {
            ready_lock.lock();
            save_proc(cur_proc, status);
            ready_lock.unlock();
            proc->status = Ready;
            wakeup_proc.remove(proc);
            wakeup_lock.unlock();
            cur_proc = proc;
            goto load_proc;
        }
    }
    wakeup_lock.unlock();
    /*
    Change thread if any of the following occurs:
    - A sleeping thread wakes up now
    - The current thread is sleeping or is marked for deletion
    - The current thread is the idle thread and there is a thread ready to run
    - The thread's quantum is up
    */
    if (!(
        (cur_proc && (cur_proc->status == Delete || cur_proc->status == Sleep)) ||
        (!ready_proc.empty() && (cur_proc == idle_proc || !((timer::time() + smp::this_cpu()->sched_off) % THREAD_QUANTUM)))
    )) return status;
    {
        ready_lock.lock();
        wakeup_lock.lock();
        save_proc(cur_proc, status);
        wakeup_lock.unlock();
        if (!ready_proc.empty()) {
            cur_proc = ready_proc.front();
            ready_proc.pop();
            cur_proc->status = Ready;
        } else
            cur_proc = idle_proc;
        ready_lock.unlock();
    }

load_proc:
    cur_proc->mapper.load();
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
