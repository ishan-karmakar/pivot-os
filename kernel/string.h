#pragma once
#include <stddef.h>

size_t strlen(const char *s);
int itoa(long long num, unsigned char* str, int len, int base);
void strrev(unsigned char* str);