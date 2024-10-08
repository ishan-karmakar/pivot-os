#include <lib/logger.hpp>
#include <magic_enum.hpp>
#include <io/stdio.hpp>
#include <frg/macros.hpp>
using namespace logger;

void logger::log(log_level log_level, const char *target, const char *format, va_list args) {
    if (log_level > LOG_LEVEL) return;
    printf("[%s] %s: ", magic_enum::enum_name(log_level).begin(), target);
    vprintf(format, args);
    printf("\n");
}

void logger::log(log_level log_level, const char *target, const char *format, ...) {
    if (log_level > LOG_LEVEL) return;
    va_list args;
    va_start(args, format);
    log(log_level, target, format, args);
    va_end(args);
}

void __assert_fail(const char *assertion, const char *file, uint32_t line, const char *func) {
    printf("%s:%u: Assertion failed in %s: '%s'", file, line, func, assertion);
    abort();
}

extern "C" {
    void FRG_INTF(log)(const char *s) {
        printf("%s\n", s);
    }

    void FRG_INTF(panic)(const char *s) {
        frg_log(s);
        abort();
    }
}