#pragma once
#include <cstddef>
#include <frg/hash_map.hpp>
#include <functional>
#include <mem/heap.hpp>

namespace cpu {
    struct status;
}

namespace syscalls {
    extern frg::hash_map<std::size_t, std::function<cpu::status*(cpu::status*)>, frg::hash<std::size_t>, heap::allocator> handlers;

    void init();
}