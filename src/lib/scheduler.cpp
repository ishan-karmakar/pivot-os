#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
#include <lib/vector.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>
using namespace scheduler;

constexpr std::size_t THREAD_STACK = 2 * PAGE_SIZE;
constexpr std::size_t THREAD_HEAP = heap::policy::sb_size;

lib::vector<process*> processes;

void scheduler::start() {
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

void process::enqueue() { processes.push_back(this); }