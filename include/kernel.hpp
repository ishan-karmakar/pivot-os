#pragma once
#include <cstddef>

constexpr size_t HUGEPAGE_SIZE = 0x200000;
constexpr size_t PAGE_SIZE = 0x1000;

template <typename L, typename R>
inline L div_ceil(L num, R dividend) {
    return (num + (dividend - 1)) / dividend;
}

template <typename L, typename R>
inline L div_floor(L num, R dividend) {
    return static_cast<size_t>(num / dividend) * dividend;
}