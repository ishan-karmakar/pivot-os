#include <lib/logger.hpp>
#include <frg/string.hpp>
#include <magic_enum.hpp>
#include <io/stdio.hpp>

namespace logger {
    void vlog(LogLevel log_level, const char *target, const char *format, va_list args) {
        if (log_level > LOG_LEVEL) return;
        // printf("[%s]", magic_enum::enum_name(log_level).begin());
        printf("[%s] %s: ", magic_enum::enum_name(log_level).begin(), target);
        vprintf(format, args);
        printf("\n");
    }

    [[noreturn]]
    void vpanic(const char *target, const char *format, va_list args) {
        error(target, format, args);
        abort();
    }

    void vassert(bool condition, const char *target, const char *format, va_list args) {
        if (!condition)
            vpanic(target, format, args);
    }

    void log(LogLevel log_level, const char *target, const char *format, ...) {
        if (log_level > LOG_LEVEL) return;
        va_list args;
        va_start(args, format);
        vlog(log_level, target, format, args);
        va_end(args);
    }

    #undef assert
    void assert(bool condition, const char *target, const char *format, ...) {
        if (condition) return;
        va_list args;
        va_start(args, format);
        vassert(condition, target, format, args);
        va_end(args);
    }

    [[noreturn]]
    void panic(const char *target, const char *format, ...) {
        va_list args;
        va_start(args, format);
        vpanic(target, format, args);
        // No need to end args since our journey ends here
    }
}

extern "C" {
    void frg_log(const char *s) {
        printf("%s\n", s);
    }

    void frg_panic(const char *s) {
        frg_log(s);
        abort();
    }
}