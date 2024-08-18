#pragma once
#include <drivers/ioapic.hpp>
#include <cstddef>
#include <atomic>

namespace pit {
    constexpr int MS_TICKS = 1193;
    extern std::atomic_size_t ticks;

    void init();
    void start(uint16_t);
    void stop();
}