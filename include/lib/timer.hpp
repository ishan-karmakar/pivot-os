#pragma once
#include <cstddef>
#include <cstdint>

namespace timer {
    extern uint8_t irq;
    void sleep(std::size_t);
    std::size_t time();
}
