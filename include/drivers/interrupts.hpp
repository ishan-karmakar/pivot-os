#pragma once
#include <cstdint>
#include <utility>

namespace interrupts {
    void set(uint8_t, uint8_t, std::pair<uint8_t, uint32_t> = { 0, 0 });

    void mask(uint8_t);
    void unmask(uint8_t);
    void eoi(uint8_t);
}