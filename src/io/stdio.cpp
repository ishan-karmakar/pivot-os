#include <frg/printf.hpp>
#include <io/stdio.hpp>
#include <cpu/cpu.hpp>
#include <io/serial.hpp>
#include <frg/spinlock.hpp>
using namespace io;

FILE *stdout = nullptr;
frg::expected<frg::format_error>(*io::print)(const char*, frg::va_struct*);

int __vfprintf_chk(FILE*, int, const char *fmt, va_list a) {
    frg::va_struct args;
    va_copy(args.args, a);
    print(fmt, &args).unwrap();

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
