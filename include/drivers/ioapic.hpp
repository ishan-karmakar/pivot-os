#pragma once
#include <cstdint>
#include <utility>

namespace ioapic {
    constexpr int LOWEST_PRIORITY = (1 << 8);
    constexpr int SMI = (0b10 << 8);
    constexpr int NMI = (0b100 << 8);
    constexpr int INIT = (0b101 << 8);
    constexpr int EXT_INT = (0b111 << 8);
    constexpr int LOGICAL_DEST = (1 << 11);
    constexpr int ACTIVE_LOW = (1 << 13);
    constexpr int LEVEL_TRIGGER = (1 << 15);
    constexpr int MASKED = (1 << 16);

    extern bool initialized;

    void init();
    void set(uint8_t, uint8_t, uint8_t, uint32_t);
    void mask(uint8_t);
    void unmask(uint8_t);
}