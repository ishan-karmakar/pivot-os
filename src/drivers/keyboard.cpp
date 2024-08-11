#include <drivers/keyboard.hpp>
#include <io/serial.hpp>
#include <util/logger.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>
#include <drivers/ioapic.hpp>
using namespace drivers;

extern "C" void keyboard_irq();

char keymap[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,
    0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

char shift_keymap[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '?', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '|', 0,
    '*', 0, ' ', 0,
    0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

void Keyboard::init(idt::IDT& idt) {
    io::inb(PORT);

    io::outb(PORT, 0xF0); // Scancode set 1
    io::outb(PORT, 2);
    check_ack();

    io::outb(STATUS, 0x20);
    while (io::inb(STATUS) & 0b10) asm ("pause");
    uint8_t config_byte = io::inb(PORT);
    logger::assert(config_byte & (1 << 6), "KEYBOARD[INIT]", "Translation is not enabled");
    io::outb(PORT, 0xF4);
    check_ack();

    idt.set_entry(IDT_ENT, 0, keyboard_irq);
    IOAPIC::set_irq(IDT_ENT, 1, 0, IOAPIC::LOWEST_PRIORITY);
    logger::info("KEYBOARD[INIT]", "Initialized PS/2 keyboard");
}

void Keyboard::check_ack() {
    logger::assert(io::inb(PORT) == 0xFA, "KEYBOARD[ACK]", "Keyboard failed to acknowledge");
}

cpu::cpu_status *cpu::keyboard_handler(cpu::cpu_status *status) {
    logger::verbose("KEYBOARD", "Keyboard handler called");
    LAPIC::eoi();
    return status;
}
