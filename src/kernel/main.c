#include <kernel/logging.h>
#include <cpu/idt.h>
#include <stdint.h>

extern uintptr_t multiboot_framebuffer_data;
extern uintptr_t multiboot_mmap_data;
extern uintptr_t multiboot_basic_meminfo;
extern uintptr_t multiboot_acpi_info;

void kernel_start(void) {
    init_qemu();
    qemu_write_string("Hello World");
    init_idt();
    asm volatile ("int $8");
    // while (1);
}