#pragma once

typedef enum log_level {
    ERROR,
    WARNING,
    INFO,
    VERBOSE,
    DEBUG
} log_level_t;

constexpr log_level_t MIN_LOG_LEVEL = DEBUG;

void log(log_level_t log_level, const char*, const char*, ...);
