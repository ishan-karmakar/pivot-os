#include <cstddef>
#include <frg/string.hpp>

std::size_t strlen(const char *c) noexcept {
    return frg::generic_strlen(c);
}

int strncmp(const char *s1, const char *s2, std::size_t n) {
    unsigned char u1, u2;

    while (n-- > 0) {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (u1 != u2)
	    return u1 - u2;
        if (u1 == '\0')
	    return 0;
    }
    return 0;
}

#ifdef __OPTIMIZE__
static const int32_t table[] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    64,
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    91,92,93,94,95,96,
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z'
};

const int32_t **__ctype_tolower_loc() { return reinterpret_cast<const int32_t**>(&table); }
#else

int isupper(int c) {
    return (unsigned) c - 'A' < 26;
}

int tolower(int c) {
    return isupper(c) ? c - 'A' + 'a' : c;
}

#endif

