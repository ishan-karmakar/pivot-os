#pragma once
#include <io/stdio.hpp>
#include <cstdint>

namespace io {
    void outb(int port, uint8_t data);
    uint8_t inb(int port);

    class serial_port : public owriter {
    public:
        using owriter::append;

        serial_port(uint16_t);
        void append(char) override;

    private:
        const uint16_t port;
    };
}