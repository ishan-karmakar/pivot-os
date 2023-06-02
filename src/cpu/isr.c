#include <stdint.h>
#include <kernel/logging.h>
#include "libc/string.h"

static const char *exception_names[] = {
    "Divide by Zero Error",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

__attribute__((noreturn))
void exception_handler(uint64_t int_no, uint64_t error_code) {
    qemu_write_string(exception_names[int_no]);
    qemu_write_char('\n');
    char buf[10];
    qemu_write_string(ultoa(error_code, buf, 10));
    qemu_write_char('\n');
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}
