#pragma once

enum LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error
};

void log(enum LogLevel level, char* target, char* msg);
