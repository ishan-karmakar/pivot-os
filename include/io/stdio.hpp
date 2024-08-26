#pragma once
#include <frg/printf.hpp>
#include <string>

namespace io {
    struct owriter {
        virtual ~owriter() = default;
        virtual void append(char) = 0;
        virtual void append(std::string_view s) { for (char c : s) append(c); }
    };

    extern owriter *writer;
}

int printf(const char*, ...);
