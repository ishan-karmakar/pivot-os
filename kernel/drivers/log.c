#include "log.h"
#include "screen.h"

void print_level_target(enum LogLevel level, char *target) {
    char* str;
    switch (level) {
        case Error:
            str = "ERROR";
            break;
        case Warn:
            str = "WARN";
            break;
        case Info:
            str = "INFO";
            break;
        case Debug:
            str = "DEBUG";
            break;
        case Trace:
            str = "TRACE";
    }

    print_char('(');
    print_string(target);
    print_string(") ");
    print_string(str);
    print_string(": ");
}

void log(enum LogLevel level, char* target, char* msg) {
    print_level_target(level, target);
    print_string(msg);
    print_char('\n');
}

void log_num(enum LogLevel level, char* target, long long num) {
    print_level_target(level, target);
    print_num(num);
    print_char('\n');
}
