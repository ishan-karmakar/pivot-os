#pragma once
#include <libc/string.h>
#include <cstddef>
#include <util/hash.hpp>

namespace util {
    class string {
    public:
        string();
        string(const char*);
        string(const char*, size_t);
        ~string();
        size_t size() const { return len - 1; }
        const char *c_str() { return *this; }
        operator const char*() const { return buffer; }
        char operator[](int idx) const { return buffer[idx]; }
        void operator=(const string& src) {
            strcpy(buffer, src.buffer);
        }

        friend bool operator==(const string& s1, const string& s2) { return !strcmp(s1, s2); }

    private:
        char *buffer;
        size_t len;
    };

    template<>
    struct hash<string> {
        size_t operator()(string);

    private:
        inline size_t shift_mix(size_t v) { return v ^ (v >> 47); }

        inline size_t load_bytes(const char *p, int n) {
            size_t result = 0;
            --n;
            do
                result = (result << 8) + static_cast<unsigned char>(p[n]);
            while (--n >= 0);
            return result;
        }

        inline size_t unaligned_load(const char *p) {
            size_t result;
            memcpy(&result, p, sizeof(result));
            return result;
        }
    };
}