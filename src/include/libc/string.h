#pragma once
#include <stddef.h>
#include <stdint.h>

void
itoa(int64_t num, char* str, int len, int base);

char *ultoa(unsigned long num, char *str, int radix);

void *
memcpy (void *dest, const void *src, size_t len);

void *
memset (void *dest, int val, size_t len);
