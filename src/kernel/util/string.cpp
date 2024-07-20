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

size_t hash<string>::operator()(const string& str) const {
    size_t hash = 5381;
    for (size_t i = 0; i < str.size(); i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}