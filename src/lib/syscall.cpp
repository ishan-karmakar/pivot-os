#include <lib/syscall.hpp>
#include <cpu/idt.hpp>
#include <lib/interrupts.hpp>
#include <lib/proc.hpp>
#include <frg/hash_map.hpp>
#include <syscall.h>
using namespace syscalls;

constexpr std::size_t VEC = 0x80;
cpu::status *syscall_handler(cpu::status*);
frg::hash_map<std::size_t, std::function<cpu::status*(cpu::status*)>, frg::hash<std::size_t>, heap::allocator> syscalls::handlers{{}};

void syscalls::init() {
    idt::handlers[intr::IRQ(VEC)].push_back([](cpu::status *status) {
        auto s = handlers[status->rdi](status);
        return s ? s : status;
    });
    logger::info("SYSCALLS", "Initialized system calls");
}

extern "C" void syscall(std::size_t, ...) {
    asm volatile ("int %0" :: "N" (VEC));
}
