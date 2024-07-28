// Memory functions
#include <cstddef>

extern "C" {
    int memcmp (const void *str1, const void *str2, size_t count) {
    const char *s1 = (const char*)str1;
    const char *s2 = (const char*)str2;

    while (count-- > 0)
        {
        if (*s1++ != *s2++)
        return s1[-1] < s2[-1] ? -1 : 1;
        }
    return 0;
    }

    void *memcpy (void *__restrict dest, const void *__restrict src, size_t len) {
    char *d = static_cast<char*>(dest);
    const char *s = static_cast<const char*>(src);
    while (len--)
        *d++ = *s++;
    return dest;
    }

    void *memset (void *dest, int val, size_t len) {
    char *ptr = static_cast<char*>(dest);
    while (len-- > 0)
        *ptr++ = val;
    return dest;
    }

    void *memmove (void *dest, const void *src, size_t len) {
    char *d = static_cast<char*>(dest);
    const char *s = static_cast<const char*>(src);
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
}