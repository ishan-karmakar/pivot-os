#pragma once
#include <cstddef>
#include <frg/bitset.hpp>

constexpr size_t HUGEPAGE_SIZE = 0x200000;
constexpr size_t PAGE_SIZE = 0x1000;

inline auto div_ceil(const auto& num, const auto& dividend) {
    return (num + (dividend - 1)) / dividend;
}

inline auto div_floor(const auto& num, const auto& dividend) {
    return static_cast<size_t>(num / dividend) * dividend;
}

inline auto round_down(const auto& num, const auto& dividend) {
    return div_floor(num, dividend) * dividend;
}

inline auto round_up(const auto& num, const auto& dividend) {
    return div_ceil(num, dividend) * dividend;
}