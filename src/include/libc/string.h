#pragma once
#include <stddef.h>

void
itoa(int num, char* str, int len, int base);

char *ultoa(unsigned long num, char *str, int radix);

void *
memcpy (void *dest, const void *src, size_t len);

void *
memset (void *dest, int val, size_t len);
