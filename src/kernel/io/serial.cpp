#include <io/serial.hpp>
#include <util/logger.hpp>

using namespace io;

void io::outb(int port, uint8_t data) {
    asm volatile ("outb %b0,%w1": :"a" (data), "Nd" (port));
}

uint8_t io::inb(int port) {
    uint8_t data;
    asm volatile ("inb %w1,%0":"=a" (data):"Nd" (port));
    return data;
}

// This can only be called ONCE
SerialPort::SerialPort(uint16_t port) : port{port} {
    outb(port + 1, 0x00);    // Disable all interrupts
	outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port + 1, 0x00);    //                  (hi byte)
	outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port + 4, 0x0B);
    outb(port + 4, 0x0F);
}

void SerialPort::append(char c) {
    while (!(inb(port + 5) & 0x20));
    outb(port, c);
}
