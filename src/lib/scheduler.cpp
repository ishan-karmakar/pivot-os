#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
#include <lib/vector.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>
#include <lib/interrupts.hpp>
#include <drivers/pit.hpp>
using namespace scheduler;

constexpr std::size_t THREAD_STACK = 2 * PAGE_SIZE;
constexpr std::size_t THREAD_HEAP = heap::policy::sb_size;

process *processes;
process *wakeup_proc;
process *cur_proc;

cpu::status *schedule(cpu::status*);

void scheduler::start() {
    if (lapic::initialized)
        idt::set_handler(intr::IRQ(lapic::timer_vec), schedule);
    else
        idt::set_handler(pit::IRQ, schedule);
}

process::process(const char *name, uintptr_t addr, bool superuser) :
    name{name},
    mapper{({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        const auto kpml4 = mapper::kmapper->data();
        for (int i = 256; i < 512; i++)
            tbl[i] = kpml4[i];
        tbl;
    })},
    vmm{({
        mapper.load();
        PAGE_SIZE;
    }), THREAD_STACK + THREAD_HEAP, superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, mapper},
    policy{vmm},
    pool{policy}
{
    ef.rsp = reinterpret_cast<uintptr_t>(vmm.malloc(THREAD_STACK)) + THREAD_STACK;
    ef.rip = addr;
    ef.int_no = 101; // Some random int code that represents thread
    ef.rflags = 0x202;
    if (superuser) {
        ef.cs = gdt::KCODE;
        ef.ss = gdt::KDATA;
    } else {
        ef.cs = gdt::UCODE;
        ef.ss = gdt::UDATA;
    }

    mapper::kmapper->load(); // Reset back to kernel's address space
    logger::info("SCHED[PROC]", "Created new process '%s'", name);
}

void add_process(process **list, process *proc) {
    if (*list)
        (*list)->prev = proc;
    proc->next = *list;
    *list = proc;
}

void delete_process(process **list, process *proc) {
    if (*list == proc)
        *list = proc->next;
    else {
        proc->prev->next = proc->next;
        proc->next->prev = proc->prev;
    }
}

void process::enqueue() {
    add_process(&processes, this);
    cur_proc = processes;
}

// WARNING: This WILL FAIL if processes is empty
cpu::status *scheduler::schedule(cpu::status *status) {
    // Check for any processes waking up
    // if (wakeup_proc && lapic::ticks >= wakeup_proc->wakeup) {
    //     // Load thread and move from wakeup processes list to regular processes list
    //     // Technically this isn't fair because this thread could be the next one run again
    //     // But I don't really care atp
    //     wakeup_proc->mapper.load();
    //     add_process(&processes, wakeup_proc);
    //     delete_process(&wakeup_proc, wakeup_proc);
    //     return &processes->ef;
    // }

    if (lapic::ticks % 100) return status;
    auto next_proc = cur_proc->next;
    if (cur_proc->status == Delete) {
        delete_process(&processes, cur_proc);
        delete cur_proc;
        cur_proc = next_proc;
    } else if (cur_proc->status == Ready || cur_proc->status == Sleep) {
        // Save status
        memcpy(&cur_proc->ef, status, sizeof(cpu::status));
        if (cur_proc->status == Sleep) {
            delete_process(&processes, cur_proc);
            add_process(&wakeup_proc, cur_proc);
        }
        cur_proc = next_proc;
    }
    if (!cur_proc) cur_proc = processes;

    cur_proc->status = Ready;
    cur_proc->mapper.load();
    return &cur_proc->ef;
}