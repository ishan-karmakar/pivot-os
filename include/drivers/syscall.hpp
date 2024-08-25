#pragma once
#include <cstddef>

namespace syscall {
    constexpr std::size_t VEC = 0x80;

    void init();
}