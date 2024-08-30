#include <io/serial.hpp>
#include <lib/logger.hpp>

using namespace io;

// This can only be called ONCE
serial_port::serial_port(uint16_t port) : port{port} {
    out<uint8_t>(port + 1, 0x00);    // Disable all interrupts
	out<uint8_t>(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	out<uint8_t>(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	out<uint8_t>(port + 1, 0x00);    //                  (hi byte)
	out<uint8_t>(port + 3, 0x03);    // 8 bits, no parity, one stop bit
	out<uint8_t>(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	out<uint8_t>(port + 4, 0x0B);
    out<uint8_t>(port + 4, 0x0F);
}

void serial_port::append(char c) {
    while (!(in<uint8_t>(port + 5) & 0x20));
    out<uint8_t>(port, c);
}
