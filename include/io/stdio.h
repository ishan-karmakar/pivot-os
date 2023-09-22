#pragma once
#include <stdarg.h>

void vprintf(const char *format, va_list args);
void printf(const char *format, ...);
void flush_screen(void);
