#include <stdint.h>
#include <stddef.h>
#include <libc/string.h>

int
memcmp (const void *str1, const void *str2, size_t count)
{
  register const unsigned char *s1 = (const unsigned char*)str1;
  register const unsigned char *s2 = (const unsigned char*)str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}

void *
memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}

void *
memset (void *dest, int val, size_t len)
{
  unsigned char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

void
strrev(char *str, size_t len)
{
    int i;
    int j;
    char a;
    for (i = 0, j = len - 1; i < j; i++, j--)
    {
            a = str[i];
            str[i] = str[j];
            str[j] = a;
    }
}

int
itoa(int64_t num, char* str, int base)
{
    int64_t sum = num;
    int i = 0;
    if (num < 0)
        sum = -sum;
    int digit;
    do
    {
            digit = sum % base;
            if (digit < 0xA)
                    str[i++] = '0' + digit;
            else
                    str[i++] = 'A' + digit - 0xA;
            sum /= base;
    }while (sum);
    if (num < 0)
        str[i++] = '-';
    strrev(str, i);
    return i;
}

int ultoa(unsigned long num, char *str, int radix) {
    int digit;
    int i = 0;

    //construct a backward string of the number.
    do {
        digit = (unsigned long)num % radix;
        if (digit < 10) 
            str[i++] = digit + '0';
        else
            str[i++] = digit - 10 + 'A';
        num /= radix;
    } while (num);

    //now reverse the string.
    strrev(str, i);
    return i;
}

static char *add_string(char *buf, char *str) {
    while (*str)
        *buf++ = *str++;
    return buf;
}

int vsprintf(char *buf, const char *c, va_list args) {
    char *start = buf;
    for (; *c; c++) {
        if (*c == '%' && *(c + 1) == '%') {
            *buf++ = '%';
            c++;
        } else if (*c != '%')
            *buf++ = *c;
        else
            switch (*++c) {
                case 's': {
                    buf = add_string(buf, va_arg(args, char*));
                    break;
                } case 'c': {
                    char ch = (char) va_arg(args, int);
                    *buf++ = ch;
                    break;
                } case 'd': {
                    buf += itoa(va_arg(args, int64_t), buf, 10);
                    break;
                } case 'u': {
                    buf += ultoa(va_arg(args, uint64_t), buf, 10);
                    break;
                } case 'x': {
                    buf = add_string(buf, "0x");
                    buf += ultoa(va_arg(args, uint64_t), buf, 16);
                    break;
                } case 'b': {
                    buf = add_string(buf, "0b");
                    buf += ultoa(va_arg(args, uint64_t), buf, 2);
                    break;
                }
            }
    }
    *buf = 0;
    return buf - start;
}

int sprintf(char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(str, fmt, args);
    va_end(args);
    return ret;
}