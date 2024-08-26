#pragma once
#include <cstdarg>
#include <cstdlib>

#define MAKE_LOG_LEVEL(fn_name, enum_name) \
[[gnu::always_inline]] inline void fn_name(const char *target, const char *format, ...) { \
    log(enum_name, target, format, __va_arg_pack()); \
}

// Safeguard against LOG_LEVEL not being set and to remove the stupid error about undefined variable
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

    void vlog(log_level, const char*, const char*, va_list);
    void log(log_level log_level, const char*, const char*, ...);

    MAKE_LOG_LEVEL(debug, DEBUG);
    MAKE_LOG_LEVEL(verbose, VERBOSE);
    MAKE_LOG_LEVEL(info, INFO);
    MAKE_LOG_LEVEL(warning, WARNING);
    MAKE_LOG_LEVEL(error, ERROR);

    [[gnu::always_inline]]
    [[noreturn]]
    inline void panic(const char *target, const char *format, ...) {
        error(target, format, __va_arg_pack());
        abort();
    }
}