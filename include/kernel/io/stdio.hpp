#pragma once
#include <frg/printf.hpp>

namespace io {
    struct OWriter {
        virtual void append(char) = 0;
        void append(const char *s) { while (*s) append(*s++); }
    };

    extern OWriter *writer;

    class CharPrinter {
    public:
        CharPrinter(OWriter *writer, frg::va_struct *args) : args{args}, writer{writer} {};
        frg::expected<frg::format_error> operator()(char);
        frg::expected<frg::format_error> operator()(const char*, size_t);
        frg::expected<frg::format_error> operator()(char, frg::format_options, frg::printf_size_mod);

    private:
        frg::va_struct *args;
        OWriter *writer;
    };
}
