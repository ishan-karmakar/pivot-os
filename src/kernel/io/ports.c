#include <io/ports.h>

void outb(int port, uint8_t data) {
    asm volatile ("outb %b0,%w1": :"a" (data), "Nd" (port));
}

uint8_t inb(int port) {
    uint8_t data;
    asm volatile ("inb %w1,%0":"=a" (data):"Nd" (port));
    return data;
}