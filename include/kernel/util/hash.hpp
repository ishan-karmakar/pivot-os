#pragma once
#include <cstddef>

namespace util {
    template<typename K>
    struct hash;

    template <typename K>
    struct hash<K*> {
        size_t operator()(K* p) const { return reinterpret_cast<size_t>(p); }
    };
    
    #define MAKE_TRIVIAL_HASH(T) \
        template <> struct hash<T> { \
            size_t operator()(T p) const { return static_cast<size_t>(p); }; \
        };

    MAKE_TRIVIAL_HASH(char);
    MAKE_TRIVIAL_HASH(unsigned char);
    MAKE_TRIVIAL_HASH(short);
    MAKE_TRIVIAL_HASH(unsigned short);
    MAKE_TRIVIAL_HASH(int);
    MAKE_TRIVIAL_HASH(unsigned int);
    MAKE_TRIVIAL_HASH(long);
    MAKE_TRIVIAL_HASH(unsigned long);
}