#include <frg/printf.hpp>
#include <io/stdio.hpp>
using namespace io;

OWriter *io::writer = nullptr;

class StringWriter : public OWriter {
public:
    StringWriter(char* buf) : buf{buf} {}
    void append(char c) override { buf[idx++] = c; }
    char *buf;
    size_t idx;

private:
};

frg::expected<frg::format_error> CharPrinter::operator()(char c) {
    writer->append(c);
    return frg::success;
}

frg::expected<frg::format_error> CharPrinter::operator()(const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        auto e = operator()(s[i]);
        if (!e) return e;
    }

    return frg::success;
}

frg::expected<frg::format_error> CharPrinter::operator()(char c, frg::format_options opts, frg::printf_size_mod size_mod) {
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

int vsprintf(char *buf, const char *fmt, va_list a) {
    frg::va_struct args;
    va_copy(args.args, a);

    StringWriter w{buf};
    CharPrinter cp{&w, &args};
    frg::printf_format(cp, fmt, &args).unwrap();

    va_end(args.args);
    return w.idx;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    va_end(args);

    return ret;
}

int vprintf(const char *fmt, va_list a) {
    frg::va_struct args;
    va_copy(args.args, a);

    CharPrinter cp{writer, &args};
    frg::printf_format(cp, fmt, &args).unwrap();

    va_end(args.args);
    return 0;
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    // TODO: Return number of characters written
    return 0;
}