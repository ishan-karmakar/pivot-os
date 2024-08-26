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
#include <set>
#include <queue>
#include <mutex>
#include <shared_mutex>
using namespace scheduler;

constexpr int THREAD_QUANTUM = 100;

struct proc_comparator {
    constexpr bool operator()(process *l, process *r) const { return l->wakeup < r->wakeup; }
};

std::queue<process*> ready_proc;
frg::simple_spinlock ready_lock;
std::set<process*, proc_comparator> wakeup_proc;
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
    if (lapic::initialized)
        idt::handlers[timer::irq].push_back(schedule);
    else
        idt::handlers[timer::irq].push_back(schedule);

    // std::size_t interval = THREAD_QUANTUM / smp::cpu_count;
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

inline process*& cur_proc() {
    return smp::this_cpu()->cur_proc;
}

static void save_proc(process*& _cur_proc, cpu::status *status) {
    if (_cur_proc && _cur_proc != idle_proc) {
        if (_cur_proc->status == Delete)
            delete _cur_proc;
        else if (_cur_proc->status == Ready || _cur_proc->status == Sleep) {
            memcpy(&_cur_proc->ef, status, sizeof(cpu::status));
            memcpy(_cur_proc->fpu_data, smp::this_cpu()->fpu_data, cpu::fpu_size);
        }

        if (_cur_proc->status == Ready)
            ready_proc.push(_cur_proc);
        else if (_cur_proc->status == Sleep)
            wakeup_proc.insert(_cur_proc);
    }
}

cpu::status *schedule(cpu::status *status) {
    process*& _cur_proc = cur_proc();
    // Check for any processes waking up
    wakeup_lock.lock();
    if (!wakeup_proc.empty()) {
        auto proc = *wakeup_proc.begin();
        if (timer::time() >= proc->wakeup) {
            ready_lock.lock();
            save_proc(_cur_proc, status);
            ready_lock.unlock();
            proc->status = Ready;
            wakeup_proc.erase(proc);
            wakeup_lock.unlock();
            _cur_proc = proc;
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
   ready_lock.lock();
    if (!(
        (_cur_proc && (_cur_proc->status == Delete || _cur_proc->status == Sleep)) ||
        (_cur_proc == idle_proc && !ready_proc.empty()) ||
        !(timer::time() % THREAD_QUANTUM)
    )) {
        ready_lock.unlock();
        return status;
    }

    {
        process *old_proc = _cur_proc;
        save_proc(_cur_proc, status);
        if (!ready_proc.empty()) {
            _cur_proc = ready_proc.front();
            ready_proc.pop();
            _cur_proc->status = Ready;
        } else
            _cur_proc = idle_proc;
        ready_lock.unlock();

        if (_cur_proc == old_proc)
            return status;
    }

load_proc:
    _cur_proc->mapper.load();
    memcpy(smp::this_cpu()->fpu_data, _cur_proc->fpu_data, cpu::fpu_size);
    return &_cur_proc->ef;
}

void proc_wrapper(void (*fn)()) {
    fn();
    syscall(SYS_exit);
}

cpu::status *scheduler::sys_exit(cpu::status *status) {
    cur_proc()->status = Delete;
    return schedule(status);
}

cpu::status *scheduler::sys_nanosleep(cpu::status *status) {
    process*& _cur_proc = cur_proc();
    _cur_proc->status = Sleep;
    _cur_proc->wakeup = timer::time() + (status->rsi / 1'000'000);
    return schedule(status);
}

void proc::sleep(std::size_t ms) {
    syscall(SYS_nanosleep, ms * 1'000'000);
}
