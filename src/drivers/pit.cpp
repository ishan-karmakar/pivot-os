#include <drivers/pit.hpp>
#include <lib/interrupts.hpp>
#include <cpu/idt.hpp>
#include <cpu/cpu.hpp>
#include <io/serial.hpp>
using namespace pit;

std::atomic_size_t pit::ticks = 0;

constexpr int IRQ = 0;
constexpr int CMD_REG = 0x43;
constexpr int DATA_REG = 0x40;

void pit::init() {
    idt::set_handler(IRQ, [](cpu::status*) {
        ticks++;
        intr::eoi(IRQ);
        return nullptr;
    });

    intr::set(intr::VEC(IRQ), IRQ);
    intr::mask(IRQ);

    io::outb(CMD_REG, 0x34);
    asm volatile ("sti");
    logger::info("PIT[INIT]", "Initialized PIT");
}

void pit::start(uint16_t d) {
    io::outb(DATA_REG, d);
    io::outb(DATA_REG, d >> 8);

    intr::unmask(IRQ);
}

void pit::stop() {
    start(0);
}