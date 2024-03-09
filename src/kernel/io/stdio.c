#include <io/stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <libc/string.h>
#include <drivers/framebuffer.h>

static char fb_buf[512];
static size_t fb_buf_pos;
char_printer_t char_printer;
atomic_flag stdio_mutex = ATOMIC_FLAG_INIT;

void flush_screen(void) {
    for (uint16_t i = 0; i < fb_buf_pos; i++)
        char_printer(fb_buf[i]);
    fb_buf_pos = 0;
}

static void add_string(char* str) {
    for (; *str != '\0'; str++)
        fb_buf[fb_buf_pos++] = *str;
}

void vprintf(const char *c, va_list args) {
    for (; *c != '\0'; c++) {
        if (*c != '%')
            fb_buf[fb_buf_pos++] = *c;
        else
            switch (*++c) {
                case 's':
                    add_string(va_arg(args, char*));
                    break;
                case 'c':
                    char ch = (char) va_arg(args, int);
                    fb_buf[fb_buf_pos++] = ch;
                    if (ch == '\n')
                        flush_screen();
                    break;
                case 'd':
                    fb_buf_pos += itoa(va_arg(args, int64_t), fb_buf + fb_buf_pos, 21, 10);
                    break;
                case 'u':
                    fb_buf_pos += ultoa(va_arg(args, uint64_t), fb_buf + fb_buf_pos, 10);
                    break;
                case 'x':
                    add_string("0x");
                    fb_buf_pos += ultoa(va_arg(args, uint64_t), fb_buf + fb_buf_pos, 16);
                    break;
                case 'b':
                    add_string("0b");
                    fb_buf_pos += ultoa(va_arg(args, uint64_t), fb_buf + fb_buf_pos, 2);
            }
        if (*c == '\n')
            flush_screen();
    }
}

void printf(const char *format, ...) {
    while (atomic_flag_test_and_set(&stdio_mutex))
        asm volatile ("pause");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    atomic_flag_clear(&stdio_mutex);
}

void printf_at(size_t x, size_t y, const char *format, ...) {
    while (atomic_flag_test_and_set(&stdio_mutex))
        asm volatile ("pause");
    size_t old_x = screen_x;
    size_t old_y = screen_y;
    screen_x = x;
    screen_y = y;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    flush_screen();
    screen_x = old_x;
    screen_y = old_y;
    atomic_flag_clear(&stdio_mutex);
}