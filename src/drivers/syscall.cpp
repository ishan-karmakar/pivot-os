#include <drivers/syscall.hpp>
#include <cpu/idt.hpp>
#include <lib/interrupts.hpp>
#include <unordered_map>
using namespace syscall;

cpu::status *syscall_handler(cpu::status*);

void syscall::init() {
    idt::handlers[intr::IRQ(VEC)].push_back(syscall_handler);
}

cpu::status *syscall_handler(cpu::status *) {
    return nullptr;
}