#pragma once
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void * memcpy (void *dest, const void *src, size_t len);
void * memset (void *dest, int val, size_t len);
int memcmp (const void *str1, const void *str2, size_t count);

int vsprintf(char*, const char*, va_list);
int sprintf(char*, const char*, ...);

#ifdef __cplusplus
}
#endif