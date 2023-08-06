#include <kernel/logging.h>
#include <io/ports.h>
#include <drivers/framebuffer.h>
#define QEMU_LOG_SERIAL_PORT 0x3F8

extern log_level_t min_log_level;

char *log_levels[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "VERBOSE",
    "DEBUG",
    "TRACE"
};

int init_qemu(void) {
    outportb(QEMU_LOG_SERIAL_PORT + 1, 0x0);
    outportb(QEMU_LOG_SERIAL_PORT + 3, 0x80);
    outportb(QEMU_LOG_SERIAL_PORT, 0xC);
    outportb(QEMU_LOG_SERIAL_PORT + 1, 0x0);
    outportb(QEMU_LOG_SERIAL_PORT + 3, 0x3);
    outportb(QEMU_LOG_SERIAL_PORT + 2, 0xC7);
    outportb(QEMU_LOG_SERIAL_PORT + 4, 0xB);
    outportb(QEMU_LOG_SERIAL_PORT + 4, 0x0F);
    return 0;
}

void qemu_write_char(char ch){
    while((inportb(QEMU_LOG_SERIAL_PORT + 5) & 0x20) == 0);
    outportb(QEMU_LOG_SERIAL_PORT, ch);
}

void log(log_level_t log_level, const char *target, const char *format, ...) {
    if (log_level > min_log_level)
        return;
    va_list args;
    va_start(args, format);
    outf("[%s] %s: ", qemu_write_char, log_levels[log_level], target);
    voutf(format, qemu_write_char, args);
    outf("\n", qemu_write_char);
    va_end(args);
    if (FRAMEBUFFER_INITIALIZED) {
        va_start(args, format);
        printf("[%s] %s: ", log_levels[log_level], target);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}
