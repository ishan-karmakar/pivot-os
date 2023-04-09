#include "log.h"
#include "screen.h"

void log(enum LogLevel level, char* target, char* msg) {
    char* level_str;
    switch (level) {
        case Error:
            level_str = "ERROR";
            break;
        case Warn:
            level_str = "WARN";
            break;
        case Info:
            level_str = "INFO";
            break;
        case Debug:
            level_str = "DEBUG";
            break;
        case Trace:
            level_str = "TRACE";
    }
    print_char('(');
    print_string(target);
    print_string(") ");
    print_string(level_str);
    print_string(": ");
    print_string(msg);
    print_char('\n');
}
