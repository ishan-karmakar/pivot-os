#include <util/string.hpp>
#include <libc/string.h>
#include <util/hash.hpp>
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

size_t hash<string>::operator()(string str) {
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