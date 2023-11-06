#include <stddef.h>
#include <libc/string.h>
#include <io/stdio.h>
#include <drivers/framebuffer.h>
#include <kernel/logging.h>

extern char fb_buf[BUF_SIZE];
extern size_t fb_buf_pos;

void flush_screen(void) {
    if (!FRAMEBUFFER_INITIALIZED)
        return;
    for (uint16_t i = 0; i < fb_buf_pos; i++)
        print_char(fb_buf[i]);
    fb_buf_pos = 0;
}

static inline void add_string(char* str) {
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
        if (fb_buf_pos > (BUF_SIZE - 1)) {
            fb_buf_pos = 0;
            log(Error, "FB", "Buffer overflow");
        }
        if (*c == '\n')
            flush_screen();
    }
}

void printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void printf_at(size_t x, size_t y, const char *format, ...) {
    screen_info_t old_info = screen_info;
    screen_info.x = x;
    screen_info.y = y;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    flush_screen();
    screen_info = old_info;
}