#include <stdint.h>
#include <kernel/logging.h>

extern uint64_t multiboot_framebuffer_data;
extern uint64_t multiboot_mmap_data;
extern uint64_t multiboot_basic_meminfo;
extern uint64_t multiboot_acpi_info;

void kernel_start(unsigned long addr) {
    init_qemu();

    while (1);
}