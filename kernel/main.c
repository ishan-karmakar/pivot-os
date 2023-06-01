#include <stdint.h>

#define QEMU_LOG_SERIAL_PORT 0x3F8

extern uint64_t multiboot_framebuffer_data;
extern uint64_t multiboot_mmap_data;
extern uint64_t multiboot_basic_meminfo;
extern uint64_t multiboot_acpi_info;

inline void outportb(int port, uint8_t data) {
    asm volatile ("outb %%al, %%dx" : : "a" (data), "d" (port));
}

inline uint8_t inportb(int port) {
    uint8_t data;
    asm volatile ("inb %%dx, %%al" : "=a" (data) : "d" (port));
    return data;
}

int init_serial(int port) {
    outportb(port + 1, 0x0);
    outportb(port + 3, 0x80);
    outportb(port, 0xC);
    outportb(port + 1, 0x0);
    outportb(port + 3, 0x3);
    outportb(port + 2, 0xC7);
    outportb(port + 4, 0xB);
    outportb(port, 0xAE);
    if (inportb(port) != 0xAE)
        return 1;
    outportb(port + 4, 0x0F);
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

void kernel_start(unsigned long addr) {
    init_serial(QEMU_LOG_SERIAL_PORT);
    qemu_write_string("Hello World");
    while (1);
}