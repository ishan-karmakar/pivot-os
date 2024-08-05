#pragma once
#include <cstddef>
#include <frg/bitset.hpp>

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

template <typename T>
inline T round_down(const T& num, const T& dividend) {
    return div_floor(num, dividend) * dividend;
}

template <typename T>
inline T round_up(const T& num, const T& dividend) {
    return div_ceil(num, dividend) * dividend;
}