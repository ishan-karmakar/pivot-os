#pragma once
#include <cstddef>
// Function must be marked as gnu::noinline or bad things could happen
#define ALIAS_FN(name) asm (".global " #name ";" #name ":");

constexpr std::size_t HUGEPAGE_SIZE = 0x200000;
constexpr std::size_t PAGE_SIZE = 0x1000;

inline constexpr std::size_t div_ceil(const std::size_t& num, const std::size_t& dividend) {
    return (num + (dividend - 1)) / dividend;
}

inline constexpr std::size_t div_floor(const std::size_t& num, const std::size_t& dividend) {
    return static_cast<std::size_t>(num / dividend);
}

inline constexpr std::size_t round_down(const std::size_t& num, const std::size_t& dividend) {
    return div_floor(num, dividend) * dividend;
}

inline constexpr std::size_t round_up(const std::size_t& num, const std::size_t& dividend) {
    return div_ceil(num, dividend) * dividend;
}