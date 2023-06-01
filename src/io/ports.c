#include <io/ports.h>

inline void outportb(int port, uint8_t data) {
    asm volatile ("outb %%al, %%dx" : : "a" (data), "d" (port));
}

inline uint8_t inportb(int port) {
    uint8_t data;
    asm volatile ("inb %%dx, %%al" : "=a" (data) : "d" (port));
    return data;
}