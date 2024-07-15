#pragma once
#include <io/stdio.hpp>
#include <stdint.h>

namespace io {
    void outb(int port, uint8_t data);
    uint8_t inb(int port);

    class SerialPort : public OWriter {
    public:
        SerialPort(uint16_t);
        void operator<<(char) override;

    private:
        uint16_t port;
    };
}