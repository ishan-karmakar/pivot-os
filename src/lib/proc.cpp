#include <lib/proc.hpp>
#include <lib/scheduler.hpp>
#include <lib/timer.hpp>
#include <cpu/gdt.hpp>
#include <cpu/smp.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <syscall.h>
#include <unistd.h>
#include <assert.h>
using namespace proc;
using namespace scheduler;

extern "C" void proc_wrapper();
std::size_t pid;

process::process(uintptr_t fn, bool superuser, std::size_t stack_size) :
    pid{++::pid},
    cpu{-1},
    auto_create{true},
    vmm{*({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        for (int i = 256; i < 512; i++) tbl[i] = mapper::kmapper->data()[i];
        auto m = new mapper::ptmapper{tbl};
        m->load();
        new vmm::vmm_t{PAGE_SIZE, stack_size + heap::policy_t::slabsize, superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, *m};
    })},
    pool{*new heap::pool_t{*new heap::policy_t{vmm}}}
{
    init(fn, superuser, stack_size);
    mapper::kmapper->load();
}

process::process(uintptr_t fn, bool superuser, vmm::vmm_t& vmm, heap::pool_t &pool, std::size_t stack_size) :
    pid{++::pid},
    cpu{-1},
    vmm{vmm},
    pool{pool},
    fpu_data{operator new(cpu::fpu_size)}
{
    init(fn, superuser, stack_size);
}

void process::init(uintptr_t fn, bool superuser, std::size_t stack_size) {
    ef.rsp = reinterpret_cast<uintptr_t>(vmm.malloc(stack_size)) + stack_size;
    ef.rip = reinterpret_cast<uintptr_t>(proc_wrapper);
    ef.rax = fn;
    ef.rflags = 0x202;
    if (superuser) {
        ef.cs = gdt::KCODE;
        ef.ss = gdt::KDATA;
    } else {
        ef.cs = gdt::UCODE | 3;
        ef.ss = gdt::UDATA | 3;
    }
}

process::~process() {
    if (auto_create) {
        delete &vmm;
        delete &pool.policy;
        delete &pool;
    }
}

void process::enqueue() {
    ready_lock.lock();
    ready_proc[cpu == -1 ? cpu : smp::cpus[cpu].id].push(this);
    ready_lock.unlock();
}

cpu::status *proc::sys_exit(cpu::status *status) {
    smp::this_cpu()->cur_proc->status = Delete;
    return schedule(status);
}

cpu::status *proc::sys_nanosleep(cpu::status *status) {
    process*& cur_proc = smp::this_cpu()->cur_proc;
    cur_proc->status = Sleep;
    cur_proc->wakeup = timer::time() + (status->rsi / 1'000'000);
    return schedule(status);
}

cpu::status *proc::sys_getpid(cpu::status *status) {
    status->rax = smp::this_cpu()->cur_proc->pid;
    return status;
}
