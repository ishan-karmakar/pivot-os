#include <util/logger.h>
#include <io/stdio.h>
#include <stdatomic.h>
#include <stdint.h>

extern uint8_t CPU;

static char *log_levels[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "VERBOSE",
    "DEBUG",
    "TRACE"
};

void log(log_level_t log_level, const char *target, const char *format, ...) {
    if (log_level > MIN_LOG_LEVEL)
        return;
    va_list args;
    va_start(args, format);
    printf("[%hhu][%s] %s: ", CPU, log_levels[log_level], target);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}