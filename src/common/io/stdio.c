#include <io/stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <libc/string.h>

static char buf[128];
char_printer_t char_printer = NULL;
atomic_flag mutex = ATOMIC_FLAG_INIT;

void flush_screen(void) {
    if (char_printer == NULL)
        return;
    char *t = buf;
    while (*t)
        char_printer(*t++);
}

int vprintf(const char *format, va_list args) {
    while (atomic_flag_test_and_set(&mutex))
        asm ("pause");
    int ret = vsprintf(buf, format, args);
    atomic_flag_clear(&mutex);
    flush_screen();
    return ret;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}