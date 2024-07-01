#include <io/stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <libc/string.h>
#include <util/logger.h>

static char buf[128];
static size_t buf_pos;
char_printer_t char_printer;
atomic_flag mutex = ATOMIC_FLAG_INIT;

void flush_screen(void) {
    for (uint16_t i = 0; i < buf_pos; i++)
        char_printer(buf[i]);
    buf_pos = 0;
}

static void add_string(char* str) {
    for (; *str != '\0'; str++)
        buf[buf_pos++] = *str;
}

void vprintf(const char *c, va_list args) {
    for (; *c != '\0'; c++) {
        if (*c != '%' || (*c == '%' && *(c + 1) == '%'))
            buf[buf_pos++] = *c;
        else
            switch (*++c) {
                case 's': {
                    add_string(va_arg(args, char*));
                    break;
                } case 'c': {
                    char ch = (char) va_arg(args, int);
                    buf[buf_pos++] = ch;
                    if (ch == '\n')
                        flush_screen();
                    break;
                } case 'd': {
                    buf_pos += itoa(va_arg(args, int64_t), buf + buf_pos, 21, 10);
                    break;
                } case 'u': {
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 10);
                    break;
                } case 'x': {
                    add_string("0x");
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 16);
                    break;
                } case 'b': {
                    add_string("0b");
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 2);
                } default: {
                    printf("\n");
                    return log(Warning, "STDIO", "Unrecognized identifier %%%c", *c);
                }
            }
        if (*c == '\n')
            flush_screen();
    }
}

void printf(const char *format, ...) {
    while (atomic_flag_test_and_set(&mutex))
        asm ("pause");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    atomic_flag_clear(&mutex);
}