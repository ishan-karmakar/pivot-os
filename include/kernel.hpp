#pragma once
#include <cstddef>

constexpr size_t HUGEPAGE_SIZE = 0x200000;
constexpr size_t PAGE_SIZE = 0x1000;

template <typename T>
inline T div_ceil(const T& num, const T& dividend) {
    return (num + (dividend - 1)) / dividend;
}

template <typename T>
inline T div_floor(const T& num, const T& dividend) {
    return static_cast<size_t>(num / dividend) * dividend;
}