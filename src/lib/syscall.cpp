#include <lib/syscall.hpp>
#include <cpu/idt.hpp>
#include <lib/interrupts.hpp>
#include <lib/scheduler.hpp>
#include <frg/hash_map.hpp>
#include <syscall.h>
using namespace syscalls;

cpu::status *syscall_handler(cpu::status*);
frg::hash_map<unsigned int, std::function<cpu::status*(cpu::status*)>, frg::hash<unsigned int>, heap::allocator> _syscalls{{}};

void syscalls::init() {
    _syscalls[SYS_exit] = scheduler::sys_exit;
    _syscalls[SYS_nanosleep] = scheduler::sys_nanosleep;
    idt::handlers[intr::IRQ(VEC)].push_back([](cpu::status *status) {
        auto s = _syscalls[status->rdi](status);
        return s ? s : status;
    });
    logger::info("SYSCALLS", "Initialized system calls");
}

extern "C" void syscall(std::size_t, ...) {
    asm volatile ("int %0" :: "N" (VEC));
}
