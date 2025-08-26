#pragma once
#include <stddef.h>

void *memset(void *dest, int val, size_t len);
int memcmp(const void *str1, const void *str2, size_t count);
void *memcpy(void *dest, const void *src, size_t len);
size_t strlen(const char *str);
int strncmp(const char *_l, const char *_r, size_t n);
void *memmove(void *dest, const void *source, size_t num);