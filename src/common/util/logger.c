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

static atomic_flag mutex = ATOMIC_FLAG_INIT;

void log(log_level_t log_level, const char *target, const char *format, ...) {
    if (log_level > MIN_LOG_LEVEL)
        return;
    // while (atomic_flag_test_and_set(&mutex))
    //     asm ("pause");
    va_list args;
    va_start(args, format);
    printf("[%u][%s] %s: ", CPU, log_levels[log_level], target);
    vprintf(format, args);
    printf("\n");
    va_end(args);
    atomic_flag_clear(&mutex);
}