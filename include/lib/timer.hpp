#pragma once
#include <cstddef>
#include <cstdint>

namespace timer {
    void sleep(std::size_t);
    std::size_t ticks();
}
