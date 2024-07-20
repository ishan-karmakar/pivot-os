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
        string(const string&);
        string& operator=(const string&);
        ~string() { delete[] buffer; }
        size_t size() const { return len - 1; }
        const char *c_str() const { return *this; }
        operator const char*() const { return buffer; }
        char operator[](int idx) const { return buffer[idx]; }

        friend bool operator==(const string& s1, const string& s2) { return !strcmp(s1, s2); }

    private:
        char *buffer;
        size_t len;
    };

    template<>
    struct hash<string> {
        size_t operator()(const string&) const;
    };
}