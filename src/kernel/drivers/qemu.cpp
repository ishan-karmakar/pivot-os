#include <drivers/qemu.hpp>
#include <io/ports.hpp>
#include <util/logger.h>

using namespace drivers;

// This can only be called ONCE
QEMUWriter::QEMUWriter() {
    outb(0x3f8 + 1, 0x00);    // Disable all interrupts
	outb(0x3f8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(0x3f8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(0x3f8 + 1, 0x00);    //                  (hi byte)
	outb(0x3f8 + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(0x3f8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(0x3f8 + 4, 0x0B);

    set_global();
    log(Info, "QEMU", "Initialized QEMU serial output");
}

void QEMUWriter::operator<<(char c) {
    while((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, c);
}
