#include <util/string.hpp>
#include <libc/string.h>
#include <util/hash.hpp>
#include <io/stdio.hpp>
using namespace util;

string::string() : buffer{nullptr}, len{0} {}

string::string(const char *str) : string{str, strlen(str)} {}

string::string(const char *str, size_t size) : len{size + 1} {
    buffer = new char[len];
    memcpy(buffer, str, len);
}

string::~string() {
    delete[] buffer;
}

string::string(const string& src) {
    len = src.size() + 1;
    buffer = new char[len];
    strcpy(buffer, src.buffer);
}

string& string::operator=(const string& src) {
    delete[] buffer;
    len = src.size() + 1;
    buffer = new char[len];
    strcpy(buffer, src.buffer);
    return *this;
}

inline size_t hash<string>::shift_mix(size_t v) const { return v ^ (v >> 47); }

inline size_t hash<string>::load_bytes(const char *p, int n) const {
    size_t result = 0;
    --n;
    do
        result = (result << 8) + static_cast<unsigned char>(p[n]);
    while (--n >= 0);
    return result;
}

inline size_t hash<string>::unaligned_load(const char *p) const {
    size_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

size_t hash<string>::operator()(const string& str) const {
    static const size_t mul = (((size_t) 0xc6a4a793UL) << 32UL)
                + (size_t) 0x5bd1e995UL;

    // Remove the bytes not divisible by the sizeof(size_t).  This
    // allows the main loop to process the data as 64-bit integers.
    const char *s = str;
    const size_t len = str.size();
    const size_t len_aligned = len & ~(size_t)0x7;
    const char* const end = s + len_aligned;
    size_t hash = 0xc70f6907UL ^ (len * mul);
    for (const char* p = s; p != end; p += 8) {
        const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
        hash ^= data;
        hash *= mul;
    }
    if ((len & 0x7) != 0) {
        const size_t data = load_bytes(end, len & 0x7);
        hash ^= data;
        hash *= mul;
    }
    hash = shift_mix(hash) * mul;
    hash = shift_mix(hash);
    return hash;
}