#pragma once
#include <cstdint>
#include <utility>

namespace intr {
    void set(uint8_t, uint8_t, std::pair<uint8_t, uint32_t> = { 0, 0 });

    void mask(uint8_t);
    void unmask(uint8_t);
    void eoi(uint8_t);

    constexpr unsigned int IRQ(unsigned int vec) { return vec - 32; }
    constexpr unsigned int VEC(unsigned int irq) { return irq + 32; }
}