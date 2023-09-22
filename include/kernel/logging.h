#pragma once
#include <stdbool.h>
#include <io/stdio.h>

typedef enum {
    Error,
    Warning,
    Info,
    Verbose,
    Debug,
    Trace
} log_level_t;

void log(log_level_t log_level, bool flush, const char*, const char*, ...);