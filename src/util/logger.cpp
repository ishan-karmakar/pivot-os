#include <util/logger.hpp>
#include <io/stdio.hpp>
#include <magic_enum.hpp>

void log(log_level_t log_level, const char *target, const char *format, ...) {
    if (log_level > MIN_LOG_LEVEL)
        return;
    va_list args;
    va_start(args, format);
    auto test = magic_enum::enum_name(log_level);
    printf("[%hhu][%s] %s: ", 0, magic_enum::enum_name(log_level).begin(), target);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

extern "C" void uacpi_kernel_log(log_level_t log_level, const char *str) {
    log(log_level, "UACPI", str);
}