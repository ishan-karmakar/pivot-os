#pragma once
#include <cstddef>

namespace syscalls {
    constexpr std::size_t VEC = 0x80;

    void init();
}