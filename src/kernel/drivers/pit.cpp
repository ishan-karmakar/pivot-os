#include <cpu/cpu.hpp>
#include <drivers/pit.hpp>
#include <io/serial.hpp>
#include <cpu/lapic.hpp>
#include <cpu/idt.hpp>
#include <drivers/ioapic.hpp>

using namespace drivers;

extern "C" void pit_irq();

volatile size_t PIT::ticks = 0;

void PIT::init(cpu::IDT& idt) {
    idt.set_entry(IDT_ENT, 0, pit_irq);
    IOAPIC::set_irq(IDT_ENT, IRQ_ENT, 0, drivers::IOAPIC::MASKED);
}

void PIT::cmd(bool bcd, uint8_t omode, uint8_t amode, uint8_t channel) {
    uint8_t final = bcd | (omode << 1) | (amode << 4) | (channel << 6);
    io::outb(CMD_REG, final);
}

void PIT::data(uint16_t data) {
    io::outb(DATA_REG, data);
    io::outb(DATA_REG, data >> 8);
}

extern "C" cpu::cpu_status *pit_handler(cpu::cpu_status *status) {
    PIT::ticks++;
    cpu::LAPIC::eoi();
    return status;
}