#include <kernel/logging.h>
#include <io/ports.h>
#define QEMU_LOG_SERIAL_PORT 0x3F8

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

void qemu_write_char(const char ch){
    while((inportb(QEMU_LOG_SERIAL_PORT + 5) & 0x20) == 0);
    outportb(QEMU_LOG_SERIAL_PORT, ch);
}

void qemu_write_string(const char *msg) {
    while (*msg != '\0') {
        qemu_write_char(*msg);
        msg++;
    }
}
