#include <drivers/qemu.hpp>
#include <frg/manual_box.hpp>
#include <io/serial.hpp>
#include <io/stdio.hpp>

static frg::manual_box<io::serial_port> port;

void qemu::init() {
    port.initialize(0x3F8);
    io::print = [](const char *format, frg::va_struct *args) {
        return frg::printf_format(io::char_printer<io::serial_port>{*port, args}, format, args);
    };
}