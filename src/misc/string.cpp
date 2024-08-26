#include <cstddef>
#include <frg/string.hpp>

extern "C" std::size_t strlen(const char *c) noexcept(true) {
    return frg::generic_strlen(c);
}