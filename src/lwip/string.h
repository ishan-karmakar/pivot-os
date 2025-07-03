#pragma once
#include <stddef.h>

static void *memset(void *dest, int val, size_t len) {
    unsigned char *ptr = dest;
    while (len-- > 0) *ptr++ = val;
    return dest;
}

static int memcmp(const void *str1, const void *str2, size_t count) {
    const unsigned char *s1 = str1;
    const unsigned char *s2 = str2;

    while (count-- > 0) {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

static void *memcpy(void *dest, const void *src, size_t len) {
    char *d = dest;
    const char *s = src;
    while (len--) *d++ = *s++;
    return dest;
}

static void *memmove(void *dest, const void *source, size_t num);
static size_t strlen(const char *str);

static int strncmp(const char *str1, const char *str2, size_t num);