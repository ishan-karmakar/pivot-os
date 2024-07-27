#include <stdint.h>
#include <stddef.h>
#include <libc/string.h>

int memcmp (const void *str1, const void *str2, size_t count) {
  register const char *s1 = (const char*)str1;
  register const char *s2 = (const char*)str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}

void *memcpy (void *__restrict dest, const void *__restrict src, size_t len) {
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}

void *memset (void *dest, int val, size_t len) {
  char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

void *memmove (void *dest, const void *src, size_t len) {
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      const char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
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

size_t
strlen(const char *str)
{
	register const char *s;

	for (s = str; *s; ++s);
	return(s - str);
}

char *
strcpy (char *dest, const char *src)
{
  return memcpy (dest, src, strlen (src) + 1);
}

int
strcmp (const char *p1, const char *p2)
{
  register const unsigned char *s1 = (const unsigned char *) p1;
  register const unsigned char *s2 = (const unsigned char *) p2;
  unsigned char c1, c2;

  do
    {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
	return c1 - c2;
    }
  while (c1 == c2);

  return c1 - c2;
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

static char *add_int(char *buf, int len, char flags, char width) {
    if (!flags) return buf + len;
    switch (flags) {
    case ' ':
        if (*buf != '-') break;
        memmove(buf + 1, buf, len);
        *buf = ' ';
        return buf + len + 1;
    case '+':
        if (*buf == '-') break;
        memmove(buf + 1, buf, len);
        *buf = '+';
        return buf + len + 1;
    case '0':
        if (!width) break;
        int num_zeroes = (width - '0') - len;
        if (num_zeroes <= 0) break;
        memmove(buf + num_zeroes, buf, len);
        memset(buf, '0', num_zeroes);
        return buf + len + num_zeroes;
    }
    return buf + len;
}

// I am not proud of this switch statement, but it works :/
int vsprintf(char *buf, const char *c, va_list args) {
    char *start = buf;
    char flags = 0;
    char width = 0;
    for (; *c; c++) {
        if (*c == '%' && *(c + 1) == '%') {
            *buf++ = '%';
            c++;
        } else if (*c != '%')
            *buf++ = *c;
        else {
            switch_start:
            // Since va_arg promotes char + short to int, %h doesn't mean anything to us
            if (*++c == 'h')
                if (*++c == 'h')
                    ++c;
            switch (*c) {
                case 's':
                    buf = add_string(buf, va_arg(args, char*));
                    break;
                case 'c':
                    *buf++ = (char) va_arg(args, int);
                    break;
                case 'p':
                    buf = add_int(buf, ultoa(va_arg(args, uintptr_t), buf, 16), flags, width);
                    break;
                case 'b': // Not in stdio printf, but here for convenience
                    buf = add_int(buf, ultoa(va_arg(args, uint64_t), buf, 2), flags, width);
                    break;
                case 'i':
                case 'd':
                    buf = add_int(buf, itoa(va_arg(args, int), buf, 10), flags, width);
                    break;
                case 'u':
                    buf = add_int(buf, ultoa(va_arg(args, unsigned int), buf, 10), flags, width);
                    break;
                case 'x':
                    buf = add_int(buf, ultoa(va_arg(args, unsigned int), buf, 16), flags, width);
                    break;
                case 'l':
                    if (*++c == 'l') c++;
                    switch (*c) {
                    case 'i':
                    case 'd':
                        buf = add_int(buf, itoa(va_arg(args, long int), buf, 10), flags, width);
                        break;
                    case 'u':
                        buf = add_int(buf, ultoa(va_arg(args, unsigned long int), buf, 10), flags, width);
                        break;
                    case 'x':
                        buf = add_int(buf, ultoa(va_arg(args, unsigned long int), buf, 16), flags, width);
                        break;
                    }
                    break;
                default:
                    if (!flags)
                        flags = *c;
                    else if (!width)
                        width = *c;
                    goto switch_start;
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