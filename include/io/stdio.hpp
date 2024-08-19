#pragma once
#include <frg/printf.hpp>
#include <utility>

namespace io {
    struct owriter {
        typedef std::pair<std::size_t, std::size_t> coord_t;
        virtual ~owriter() = default;
        virtual coord_t get_pos() { return { 0, 0 }; }
        virtual void set_pos(coord_t) {}
        virtual coord_t get_constraints() { return { 0, 0 }; }
        virtual void append(char) {}
        void append(const char *s) { while (*s) append(*s++); }
        virtual void clear() {}
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
