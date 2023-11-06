#pragma once
#include <stdarg.h>
#include <stddef.h>

void vprintf(const char *format, va_list args);
void printf(const char *format, ...);
void printf_at(size_t x, size_t y, const char *format, ...);
void flush_screen(void);
