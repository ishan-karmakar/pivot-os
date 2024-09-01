#pragma once
#include <cstdarg>
#include <cstdlib>

#ifndef LOG_LEVEL
#define LOG_LEVEL INFO
#endif

namespace logger {
    enum log_level {
        ERROR,
        WARNING,
        INFO,
        VERBOSE,
        DEBUG
    };

    void log(log_level, const char*, const char*, va_list);
    void log(log_level, const char*, const char*, ...);

#define MAKE_LOG_LEVEL(fn, e) \
    inline void fn(const char *t, const char *f, ...) { \
        va_list l; \
        va_start(l, f); \
        log(e, t, f, l); \
        va_end(l); \
    }

    MAKE_LOG_LEVEL(debug, DEBUG);
    MAKE_LOG_LEVEL(verbose, VERBOSE);
    MAKE_LOG_LEVEL(info, INFO);
    MAKE_LOG_LEVEL(warning, WARNING);
    MAKE_LOG_LEVEL(error, ERROR);
#undef MAKE_LOG_LEVEL

    [[noreturn]]
    inline void panic(const char *target, const char *format, ...) {
        va_list l;
        va_start(l, format);
        error(target, format, l);
        abort();
    }
}