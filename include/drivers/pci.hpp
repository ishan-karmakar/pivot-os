#pragma once
#include <cstdint>

namespace pci {
    void init();
    uint32_t read(uint8_t, uint8_t, uint8_t, uint8_t);
    void write(uint32_t);
}