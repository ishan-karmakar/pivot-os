#include <kernel/logging.h>
#include <io/stdio.h>
#include <cpu/lapic.h>

static char *log_levels[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "VERBOSE",
    "DEBUG",
    "TRACE"
};

void log(log_level_t log_level, const char *target, const char *format, ...) {
    if (log_level > min_log_level)
        return;
    va_list args;
    va_start(args, format);
    printf("[%s][%u] %s: ", log_levels[log_level], apic_initialized() ? get_apic_id() : 0, target);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}