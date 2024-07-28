#include <frg/printf.hpp>
#include <io/stdio.hpp>
using namespace io;

OWriter *CharPrinter::writer = nullptr;

frg::expected<frg::format_error> CharPrinter::operator()(char c) {
    writer->append(c);
    return frg::success;
}

frg::expected<frg::format_error> CharPrinter::operator()(const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        auto e = operator()(s[i]);
        if (e) return e;
    }

    return frg::success;
}

frg::expected<frg::format_error> CharPrinter::operator()(char c, frg::format_options& opts, frg::printf_size_mod size_mod) {
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

    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'e':
    case 'E':
        frg::do_printf_floats(*writer, c, opts, size_mod, args);
        break;
    };

    return frg::success;
}

int printf(const char *fmt, ...) {
    frg::va_struct args;
    va_start(args.args, fmt);
    CharPrinter cp{&args};
    frg::printf_format(cp, fmt, &args);
    va_end(args.args);

    // TODO: Return number of characters written
    return 0;
}