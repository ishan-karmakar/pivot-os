#include <stdint.h>
#include <kernel/logging.h>
#include <cpu/idt.h>

void kernel_start(void) {
    init_qemu();
    init_idt();
    // qemu_write_string("Initialized IDT");

    while (1);
}