#include <lib/logger.hpp>
#include <magic_enum.hpp>
#include <cpu/smp.hpp>
#include <io/stdio.hpp>

namespace logger {
    void vlog_(LogLevel log_level, const char *target, const char *format, va_list args) {
        printf("[%lu][%s] %s: ", smp::cpu_id(), magic_enum::enum_name(log_level).begin(), target);
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

    void log_(LogLevel log_level, const char *target, const char *format, ...) {
        va_list args;
        va_start(args, format);
        vlog_(log_level, target, format, args);
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