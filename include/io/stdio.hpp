#pragma once
#include <frg/printf.hpp>
#include <utility>

namespace io {
    struct owriter {
        virtual ~owriter() = default;
        virtual void append(char) {}
        void append(const char *s) { while (*s) append(*s++); }
    };

    extern owriter *writer;

    class char_printer {
    public:
        char_printer(owriter *writer, frg::va_struct *args) : args{args}, writer{writer} {};
        frg::expected<frg::format_error> operator()(char);
        frg::expected<frg::format_error> operator()(const char*, std::size_t);
        frg::expected<frg::format_error> operator()(char, frg::format_options, frg::printf_size_mod);

    private:
        frg::va_struct *args;
        owriter *writer;
    };
}

int printf(const char*, ...);
