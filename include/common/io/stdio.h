#pragma once
#include <stdarg.h>
#include <stddef.h>

typedef void (*char_printer_t)(char);
extern char_printer_t char_printer;

void printf(const char *format, ...);
void vprintf(const char *c, va_list args);
void flush_screen(void);