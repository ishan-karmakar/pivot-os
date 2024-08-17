#include <drivers/pit.hpp>
#include <drivers/interrupts.hpp>
#include <cpu/idt.hpp>
#include <cpu/cpu.hpp>
#include <io/serial.hpp>
using namespace pit;

volatile std::atomic_size_t pit::ticks = 0;

constexpr int IRQ = 0;
constexpr int CMD_REG = 0x43;
constexpr int DATA_REG = 0x40;

void pit::init() {
    auto [handler, vector] = idt::allocate_handler(IRQ);
    handler = [](cpu::status *status) {
        ticks++;
        interrupts::eoi(IRQ);
        return status;
    };

    interrupts::set(vector, IRQ);
    interrupts::mask(IRQ);

    io::outb(CMD_REG, 0x34);
    logger::info("PIT[INIT]", "Initialized PIT");
}

void pit::start(uint16_t d) {
    logger::verbose("PIT[START]", "Starting timer with divisor %hu", d);
    io::outb(DATA_REG, d);
    io::outb(DATA_REG, d >> 8);

    interrupts::unmask(IRQ);
}

void pit::stop() {
    start(0);
}