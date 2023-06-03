#include <kernel/logging.h>

void kernel_start(void) {
    init_qemu();
    qemu_write_string("Hello World\n");
    while (1);
}