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
        const char *c_str() const { return *this; }
        operator const char*() const { return buffer; }
        char operator[](int idx) const { return buffer[idx]; }
        string(const string&);
        string& operator=(const string&);

        friend bool operator==(const string& s1, const string& s2) { return !strcmp(s1, s2); }

    private:
        char *buffer;
        size_t len;
    };

    template<>
    struct hash<string> {
        size_t operator()(const string&) const;

    private:
        inline size_t shift_mix(size_t) const;
        inline size_t load_bytes(const char*, int) const;
        inline size_t unaligned_load(const char*) const;
    };
}