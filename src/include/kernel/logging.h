#pragma once

typedef enum {
    Error,
    Warning,
    Info,
    Verbose,
    Debug,
    Trace
} log_level_t;

int init_qemu(void);
void qemu_write_char(char);
void log(log_level_t log_level, const char*, const char*, ...);