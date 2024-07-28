#pragma once
#include <frg/printf.hpp>

namespace io {
    struct OWriter {
        virtual void append(char) = 0;
        virtual void append(const char *s) {
            while (*s) append(*s++);
        }
    };

    class CharPrinter {
    public:
        static OWriter *writer;
        CharPrinter(frg::va_struct *args) : args{args} {};

    private:
        frg::expected<frg::format_error> operator()(char);
        frg::expected<frg::format_error> operator()(const char*, size_t);
        frg::expected<frg::format_error> operator()(char, frg::format_options&, frg::printf_size_mod);
        frg::va_struct *args;
    };
}
