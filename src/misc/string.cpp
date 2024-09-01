#include <cstddef>
#include <frg/string.hpp>

std::size_t strlen(const char *c) noexcept {
    return frg::generic_strlen(c);
}

int strncmp(const char *s1, const char *s2, std::size_t n) {
    unsigned char u1, u2;

    while (n-- > 0)
        {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (u1 != u2)
        return u1 - u2;
        if (u1 == '\0')
        return 0;
        }
    return 0;
}

int isupper(int c) {
    return (unsigned) c - 'A' < 26;
}

int tolower(int c) {
    return isupper(c) ? c - 'A' + 'a' : c;
}

const char *strchr(const char *s, int c) noexcept {
    do {
        if (*s == c)
        {
        return (char*)s;
        }
    } while (*s++);
    return (0);
}