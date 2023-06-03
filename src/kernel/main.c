#include <kernel/logging.h>
#include <cpu/idt.h>

void kernel_start(void) {
    init_qemu();
    init_idt();
    asm volatile ("int $2");
    while (1);
}