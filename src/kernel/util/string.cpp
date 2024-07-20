#include <util/string.hpp>
#include <util/hash.hpp>
#include <io/stdio.hpp>
using namespace util;

String::String() : buffer{nullptr}, len{0} {}

String::String(const char *str) : String{str, strlen(str)} {}

String::String(const char *str, size_t size) : len{size + 1} {
    buffer = new char[len];
    memcpy(buffer, str, size);
    buffer[size] = 0;
}

String::String(const String& src) {
    len = src.size() + 1;
    buffer = new char[len];
    strcpy(buffer, src.buffer);
}

String& String::operator=(const String& src) {
    delete[] buffer;
    len = src.size() + 1;
    buffer = new char[len];
    strcpy(buffer, src.buffer);
    return *this;
}

size_t hash<String>::operator()(const String& str) const {
    size_t hash = 5381;
    for (size_t i = 0; i < str.size(); i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}