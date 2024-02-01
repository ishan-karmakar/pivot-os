#pragma once

#include <stddef.h>
#include <stdint.h>

int itoa(int64_t num, char* str, int len, int base);

int ultoa(unsigned long num, char *str, int radix);

void * memcpy (void *dest, const void *src, size_t len);

void * memset (void *dest, int val, size_t len);

int memcmp (const void *str1, const void *str2, size_t count);

char *strcpy(char *dst, const char *src);