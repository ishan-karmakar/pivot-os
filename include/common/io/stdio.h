#pragma once
#include <stdarg.h>
#include <stddef.h>

typedef void (*char_printer_t)(unsigned char);
extern char_printer_t char_printer;

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *format, ...);
int vprintf(const char *c, va_list args);

#ifdef __cplusplus
}
#endif