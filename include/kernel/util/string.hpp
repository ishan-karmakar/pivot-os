#pragma once
#include <libc/string.h>
#include <cstddef>
#include <util/hash.hpp>

namespace util {
    class String {
    public:
        String();
        String(const char*);
        String(const char*, size_t);
        String(const String&);
        String& operator=(const String&);
        ~String() { delete[] buffer; }
        size_t size() const { return len - 1; }
        const char *c_str() const { return *this; }
        operator const char*() const { return buffer; }
        char operator[](int idx) const { return buffer[idx]; }

        friend bool operator==(const String& s1, const String& s2) { return !strcmp(s1, s2); }

    private:
        char *buffer;
        size_t len;
    };

    template<>
    struct hash<String> {
        size_t operator()(const String&) const;
    };
}