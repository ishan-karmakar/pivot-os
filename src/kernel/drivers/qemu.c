/*
This is only used in the initial phase of init_kernel
where framebuffer is not initialized, so data is logged
through the qemu serial port. After framebuffer is
initialized, the serial port is no longer used
*/

#include <drivers/qemu.h>
#include <io/ports.h>
#include <io/stdio.h>
#include <util/logger.h>

void qemu_write_char(const char ch){
    while((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, ch);
}

void init_qemu(void) {
    outb(0x3f8 + 1, 0x00);    // Disable all interrupts
	outb(0x3f8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(0x3f8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(0x3f8 + 1, 0x00);    //                  (hi byte)
	outb(0x3f8 + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(0x3f8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(0x3f8 + 4, 0x0B);
    char_printer = qemu_write_char;
    log(Info, "QEMU", "Initialized QEMU serial port");
}
