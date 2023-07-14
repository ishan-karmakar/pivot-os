#include <libc/string.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *str) {
        const char *s;
        for (s = str; *s; ++s)
                ;
        return (s - str);
}

void
strrev(char *str)
{
        int i;
        int j;
        char a;
        unsigned len = strlen(str);
        for (i = 0, j = len - 1; i < j; i++, j--)
        {
                a = str[i];
                str[i] = str[j];
                str[j] = a;
        }
}

int
strcmp (const char *p1, const char *p2)
{
  const unsigned char *s1 = (const unsigned char *) p1;
  const unsigned char *s2 = (const unsigned char *) p2;
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

void
itoa(int64_t num, char* str, int len, int base)
{
        int64_t sum = num;
        int i = 0;
        if (num < 0)
            sum = -sum;
        int digit;
        if (len == 0)
                return;
        do
        {
                digit = sum % base;
                if (digit < 0xA)
                        str[i++] = '0' + digit;
                else
                        str[i++] = 'A' + digit - 0xA;
                sum /= base;
        }while (sum && (i < (len - 1)));
        if (i == (len - 1) && sum)
                return;
        if (num < 0)
            str[i++] = '-';
        str[i] = '\0';
        strrev(str);
}

char *ultoa(unsigned long num, char *str, int radix) {
    char temp[33];  //an int can only be 16 bits long
                    //at radix 2 (binary) the string
                    //is at most 16 + 1 null long.
    int temp_loc = 0;
    int digit;
    int str_loc = 0;

    //construct a backward string of the number.
    do {
        digit = (unsigned long)num % radix;
        if (digit < 10) 
            temp[temp_loc++] = digit + '0';
        else
            temp[temp_loc++] = digit - 10 + 'A';
        num /= radix;
    } while ((unsigned long)num > 0);

    temp_loc--;


    //now reverse the string.
    while ( temp_loc >=0 ) {// while there are still chars
        str[str_loc++] = temp[temp_loc--];    
    }
    str[str_loc] = 0; // add null termination.

    return str;
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