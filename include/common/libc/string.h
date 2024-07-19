#pragma once
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset (void *__s, int __c, size_t __n);
void *memmove (void *__dest, const void *__src, size_t __n);
int memcmp (const void *__s1, const void *__s2, size_t __n);
void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n);

size_t strlen (const char *__s);
char *strcpy (char*, const char*);
int strcmp (const char *__s1, const char *__s2);

int vsprintf(char*, const char*, va_list);
int sprintf(char*, const char*, ...);

#ifdef __cplusplus
}
#endif