#include "mem.h"
#include <stdint.h>

uintptr_t free_mem_addr = 0x10000000;

void* malloc(size_t s) {
    uint32_t pages = s / 0x1000;
    if (s % 4096)
        pages++;
    void* addr = (void*)(uintptr_t) free_mem_addr;
    free_mem_addr += pages * 0x1000;
    return addr;
}

void free(void*) {};

void* realloc(void*, size_t s) { return malloc(s); }

void *
memset (void *dest, register int val, register size_t len)
{
  register unsigned char *ptr = (unsigned char*)dest;
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
