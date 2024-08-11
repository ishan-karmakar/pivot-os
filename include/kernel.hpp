#pragma once
#include <cstddef>

constexpr size_t HUGEPAGE_SIZE = 0x200000;
constexpr size_t PAGE_SIZE = 0x1000;

inline constexpr size_t div_ceil(const size_t& num, const size_t& dividend) {
    return (num + (dividend - 1)) / dividend;
}

inline constexpr size_t div_floor(const size_t& num, const size_t& dividend) {
    return static_cast<size_t>(num / dividend);
}

inline constexpr size_t round_down(const size_t& num, const size_t& dividend) {
    return div_floor(num, dividend) * dividend;
}

inline constexpr size_t round_up(const size_t& num, const size_t& dividend) {
    return div_ceil(num, dividend) * dividend;
}