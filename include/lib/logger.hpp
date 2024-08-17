#pragma once
#include <cstdarg>

// Safeguard against LOG_LEVEL not being set and to remove the stupid error about undefined variable
#ifndef LOG_LEVEL
#define LOG_LEVEL INFO
#endif

#define MAKE_LOG_LEVEL(fn_name, enum_name) \
[[gnu::always_inline]] inline void fn_name(const char *target, const char *format, ...) { \
    log(enum_name, target, format, __va_arg_pack()); \
}

enum LogLevel {
    ERROR,
    WARNING,
    INFO,
    VERBOSE,
    DEBUG
};

namespace logger {
    void vlog(LogLevel, const char*, const char*, va_list);
    [[noreturn]] void vpanic(const char*, const char*, va_list);
    void vassert(bool, const char*, const char*, va_list);

    void log(LogLevel log_level, const char*, const char*, ...);
    [[noreturn]] void panic(const char*, const char*, ...);
    void assert(bool, const char*, const char*, ...);

    MAKE_LOG_LEVEL(debug, DEBUG);
    MAKE_LOG_LEVEL(verbose, VERBOSE);
    MAKE_LOG_LEVEL(info, INFO);
    MAKE_LOG_LEVEL(warning, WARNING);
    MAKE_LOG_LEVEL(error, ERROR);
}