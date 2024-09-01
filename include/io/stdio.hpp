#pragma once
#include <frg/printf.hpp>

namespace io {
    template <typename T>
    concept Printer = requires (T v, char c, const char *str, std::string_view sv) {
        v.append(c);
        v.append(str);
        v.append(sv);
    };

    template <Printer P>
    struct char_printer {
        char_printer(P& writer, frg::va_struct *args) : writer{writer}, args{args} {};

        frg::expected<frg::format_error> operator()(char c) {
            writer.append(c);
            return frg::success;
        }

        frg::expected<frg::format_error> operator()(const char *s, std::size_t n) {
            writer.append(std::string_view{s, n});
            return frg::success;
        }

        frg::expected<frg::format_error> operator()(char c, frg::format_options opts, frg::printf_size_mod size_mod) {
            switch (c) {
            case 'p':
            case 'c':
            case 's':
                frg::do_printf_chars(writer, c, opts, size_mod, args);
                break;

            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
                frg::do_printf_ints(writer, c, opts, size_mod, args);
                break;
            };

            return frg::success;
        }

    private:
        P& writer;
        frg::va_struct *args;
    };

    extern frg::expected<frg::format_error>(*print)(const char*, frg::va_struct*);
}

int printf(const char*, ...);
