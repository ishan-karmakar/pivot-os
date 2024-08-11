#pragma once
#include <cstdarg>

#define MAKE_LOG_LEVEL(lower_level, upper_level) \
inline void lower_level(const char *target, const char *format, ...) { \
    va_list args; \
    va_start(args, format); \
    vlog(upper_level, target, format, args); \
    va_end(args); \
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
    void panic(const char*, const char*, ...);
    void assert(bool, const char*, const char*, ...);

    MAKE_LOG_LEVEL(debug, DEBUG);
    MAKE_LOG_LEVEL(verbose, VERBOSE);
    MAKE_LOG_LEVEL(info, INFO);
    MAKE_LOG_LEVEL(warning, WARNING);
    MAKE_LOG_LEVEL(error, ERROR);
}