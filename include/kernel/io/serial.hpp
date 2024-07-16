#pragma once
#include <io/stdio.hpp>
#include <cstdint>

namespace io {
    void outb(int port, uint8_t data);
    uint8_t inb(int port);

    class SerialPort : public OWriter {
    public:
        SerialPort(uint16_t);
        void operator<<(char) override;

    private:
        const uint16_t port;
    };
}