#pragma once
#include <stddef.h>

static void memset() {}
static void *memmove(void *dest, const void *source, size_t num) { return NULL; }
static size_t strlen(const char *str) { return 0; }

static int strncmp(const char *str1, const char *str2, size_t num) { return 0; }