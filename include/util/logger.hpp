#pragma once

enum LogLevel {
    ERROR,
    WARNING,
    INFO,
    VERBOSE,
    DEBUG
};

constexpr LogLevel MIN_LOG_LEVEL = DEBUG;

void log(LogLevel log_level, const char*, const char*, ...);
void panic(const char*, const char *, ...);