#include <drivers/pic.hpp>
#include <io/serial.hpp>
#include <lib/logger.hpp>
#define PIC_ASSERT(n) logger::assert(initialized, "PIC[" n "]", "PIC is not initialized");

using namespace pic;

constexpr int PIC1 = 0x20;
constexpr int PIC2 = 0xA0;
constexpr int PIC1_DATA = PIC1 + 1;
constexpr int PIC2_DATA = PIC2 + 1;
constexpr int PIC_EOI = 0x20;

void pic::init() {
    asm volatile ("cli");
    io::out<uint8_t>(PIC1, 0x10 | 0x1);
    io::out<uint8_t>(PIC2, 0x10 | 0x1);
    io::out<uint8_t>(PIC1_DATA, 0x20);
    io::out<uint8_t>(PIC2_DATA, 0x20 + 8);
    io::out<uint8_t>(PIC1_DATA, 4);
    io::out<uint8_t>(PIC2_DATA, 2);
    io::out<uint8_t>(PIC1_DATA, 1);
    io::out<uint8_t>(PIC2_DATA, 1);
    disable();

    logger::info("PIC", "Initialized 8259 PIC");
}

void pic::eoi(uint8_t irq) {
    if (irq < 8)
        io::out<uint8_t>(PIC1, PIC_EOI);
    io::out<uint8_t>(PIC2, PIC_EOI);
}

void pic::mask(uint8_t irq) {
    uint16_t port;
    if (irq < 8)
        port = PIC1_DATA;
    else {
        port = PIC2_DATA;
        irq -= 8;
    }

    io::out<uint8_t>(port, io::in<uint8_t>(port) | (1 << irq));
}

void pic::unmask(uint8_t irq) {
    uint16_t port;
    if (irq < 8)
        port = PIC1_DATA;
    else {
        port = PIC2_DATA;
        irq -= 8;
    }

    io::out<uint8_t>(port, io::in<uint8_t>(port) & ~(1 << irq));
}

void pic::disable() {
    io::out<uint8_t>(PIC1_DATA, 0xFF);
    io::out<uint8_t>(PIC2_DATA, 0xFF);
    logger::verbose("PIC[DISABLE]", "Disabled 8259 PIC");
}