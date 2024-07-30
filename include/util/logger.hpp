#pragma once
#define MIN_LOG_LEVEL Debug

typedef enum log_level {
    Error,
    Warning,
    Info,
    Verbose,
    Debug
} log_level_t;

void log(log_level_t log_level, const char*, const char*, ...);
