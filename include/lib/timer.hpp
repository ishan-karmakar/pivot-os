#pragma once
#include <cstddef>
#include <cstdint>

namespace timer {
    extern uint8_t irq;
    extern "C" void sleep(std::size_t);
    extern "C" std::size_t time();
}
