#pragma once
#include <cstddef>
#include <atomic>

namespace pit {
    constexpr int IRQ = 0;
    constexpr int MS_TICKS = 1193;
    extern std::atomic_size_t ticks;

    void init();
    void start(uint16_t);
    void stop();
}