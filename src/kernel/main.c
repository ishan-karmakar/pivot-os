#include <kernel/logging.h>
#include <cpu/idt.h>
#include <stdint.h>

extern uintptr_t multiboot_framebuffer_data;
extern uintptr_t multiboot_mmap_data;
extern uintptr_t multiboot_basic_meminfo;
extern uintptr_t multiboot_acpi_info;

__attribute__((noreturn))
void hcf(void) {
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}

void kernel_start(uintptr_t addr, uint64_t magic) {
    init_qemu();
    qemu_write_string("Loaded into kernel\n");
    init_idt();
    qemu_write_string("Initialized IDT\n");
    asm volatile ("int $8");
    qemu_write_string("Returned back to the kernel\n");
    while (1);
}