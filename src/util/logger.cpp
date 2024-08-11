#include <util/logger.hpp>
#include <io/stdio.hpp>
#include <frg/string.hpp>
#include <magic_enum.hpp>

namespace logger {
    void vlog(LogLevel log_level, const char *target, const char *format, va_list args) {
        if (log_level > LOG_LEVEL) return;
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

    void panic(const char *target, const char *format, ...) {
        va_list args;
        va_start(args, format);
        vpanic(target, format, args);
        // No need to end args since our journey ends here
    }
}

extern "C" void uacpi_kernel_log(LogLevel log_level, const char *str) {
    // I should find a faster way but I don't care
    // I want to remove the end newline but str is constant
    size_t len = frg::generic_strlen(str);
    char *buffer = new char[len];
    memcpy(buffer, str, --len);
    buffer[len] = 0;
    logger::vlog(log_level, "UACPI", buffer, va_list{});
    delete[] buffer;
}