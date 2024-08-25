#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>
#include <lib/interrupts.hpp>
#include <drivers/pit.hpp>
#include <set>
#include <deque>
using namespace scheduler;

struct proc_comparator {
    constexpr bool operator()(process *l, process *r) const { return l->wakeup < r->wakeup; }
};

std::deque<process*> ready_proc;
std::set<process*, proc_comparator> wakeup_proc;
process *cur_proc; // Move to CPU info
process *idle_proc;

cpu::status *schedule(cpu::status*);

[[gnu::naked]]
void idle() { while(1) asm ("pause"); }

void scheduler::init() {
    idle_proc = new process{"idle", idle, true, PAGE_SIZE, 0};
}

void scheduler::start() {
    if (lapic::initialized)
        idt::handlers[intr::IRQ(lapic::timer_vec)].push_back(schedule);
    else
        idt::handlers[pit::IRQ].push_back(schedule);
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
    pool{policy}
{
    ef.rsp = reinterpret_cast<uintptr_t>(vmm.malloc(stack_size)) + stack_size;
    ef.rip = reinterpret_cast<uintptr_t>(addr);
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
    ready_proc.push_back(this);
}

static void save_proc(cpu::status *status) {
    if (cur_proc && cur_proc != idle_proc) {
        if (cur_proc->status == Delete)
            delete cur_proc;
        else if (cur_proc->status == Ready || cur_proc->status == Sleep)
            memcpy(&cur_proc->ef, status, sizeof(cpu::status));

        if (cur_proc->status == Ready)
            ready_proc.push_back(cur_proc);
        else if (cur_proc->status == Sleep)
            wakeup_proc.insert(cur_proc);
    }
}

cpu::status *schedule(cpu::status *status) {
    // Check for any processes waking up
    if (wakeup_proc.size() > 0) {
        auto proc = *wakeup_proc.begin();
        if (lapic::ticks >= proc->wakeup) {
            save_proc(status);
            proc->status = Ready;
            ready_proc.push_back(proc);
            wakeup_proc.erase(proc);
            return &proc->ef;
        }
    }
    if (lapic::ticks % 100) return status;
    save_proc(status);
    if (ready_proc.size() > 0) {
        cur_proc = ready_proc[0];
        ready_proc.pop_front();
        cur_proc->status = Ready;
        cur_proc->mapper.load();
        return &cur_proc->ef;
    } else {
        cur_proc = idle_proc;
        idle_proc->mapper.load();
        return &idle_proc->ef;
    }
}