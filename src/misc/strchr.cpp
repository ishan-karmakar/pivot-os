extern "C" const char *strchr(const char *s, int c) noexcept {
    do {
        if (*s == c)
        {
        return (char*)s;
        }
    } while (*s++);
    return (0);
}
