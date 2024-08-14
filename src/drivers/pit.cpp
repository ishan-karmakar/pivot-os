#include <cpu/cpu.hpp>
#include <drivers/pit.hpp>
#include <io/serial.hpp>
#include <drivers/lapic.hpp>
#include <cpu/idt.hpp>
#include <drivers/ioapic.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/tables.h>
#include <mem/mapper.hpp>
#include <util/logger.hpp>

using namespace drivers;

volatile size_t PIT::ticks = 0;
bool PIT::initialized = false;

void PIT::init() {
    if (initialized) return;
    // idt.set_entry(IDT_ENT, 0, (void*) &pit_irq);
    IOAPIC::set_irq(IDT_ENT, IRQ_ENT, 0, IOAPIC::MASKED);
    drivers::PIT::cmd(false, 0b010, 0b11, 0);
    drivers::PIT::data(drivers::PIT::MS_TICKS / 10);
    initialized = true;
    logger::info("PIT", "Initialized PIT");
}

void PIT::cmd(bool bcd, uint8_t omode, uint8_t amode, uint8_t channel) {
    uint8_t final = bcd | (omode << 1) | (amode << 4) | (channel << 6);
    io::outb(CMD_REG, final);
}

void PIT::data(uint16_t data) {
    io::outb(DATA_REG, data);
    io::outb(DATA_REG, data >> 8);
}

extern "C" cpu::status *pit_handler(cpu::status *status) {
    // PIT::ticks++; // FIXME: Use atomic
    LAPIC::eoi();
    return status;
}

// Microseconds, not milliseconds
void uacpi_kernel_stall(uacpi_u8 ms) {
    drivers::IOAPIC::init();
    PIT::init();
    LAPIC::bsp_init();
    PIT::ticks = 0;
    asm volatile ("sti");
    PIT::enable();
    while (PIT::ticks < (ms / 100)) asm ("pause");
    PIT::disable();
}