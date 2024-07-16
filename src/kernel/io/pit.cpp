#include <cpu/cpu.hpp>
#include <io/pit.hpp>
#include <io/serial.hpp>
#define CMD_REG 0x43

using namespace io;

extern "C" void pit_irq();

void PIT::init(cpu::IDT& idt) {
    idt.set_entry(PIT_IDT_ENT, 0, pit_irq);
}

void PIT::cmd(bool bcd, uint8_t omode, uint8_t amode, uint8_t channel) {
    uint8_t final = bcd | (omode << 1) | (amode << 4) | (channel << 6);
    io::outb(CMD_REG, final);
}

extern "C" cpu::cpu_status *pit_handler(cpu::cpu_status*) {
    PIT::ticks++;
}