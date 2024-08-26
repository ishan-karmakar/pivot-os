#include <frg/printf.hpp>
#include <io/stdio.hpp>
#include <cpu/cpu.hpp>
#include <io/serial.hpp>
#include <frg/manual_box.hpp>
using namespace io;

owriter *io::writer = nullptr;
FILE *stdout = nullptr;

struct char_printer {
    char_printer(frg::va_struct *args) : args{args} {};

    frg::expected<frg::format_error> operator()(char c) {
        writer->append(c);
        return frg::success;
    }

    frg::expected<frg::format_error> operator()(const char *s, std::size_t n) {
        writer->append(std::string_view{s, n});

        return frg::success;
    }

    frg::expected<frg::format_error> operator()(char c, frg::format_options opts, frg::printf_size_mod size_mod) {
        switch (c) {
        case 'p':
        case 'c':
        case 's':
            frg::do_printf_chars(*writer, c, opts, size_mod, args);
            break;
        
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
            frg::do_printf_ints(*writer, c, opts, size_mod, args);
            break;
        };

        return frg::success;
    }

private:
    frg::va_struct *args;
};

int __vfprintf_chk(FILE*, int, const char *fmt, va_list a) {
    frg::va_struct args;
    va_copy(args.args, a); // PROBLEM
    char_printer cp{&args};
    frg::printf_format(cp, fmt, &args).unwrap();

    va_end(args.args);
    return 0;
}

int vprintf(const char *fmt, va_list a) {
    return __vfprintf_chk(stdout, 0, fmt, a);
}

int __printf_chk(int, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}
