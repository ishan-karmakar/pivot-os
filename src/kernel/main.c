#include <kernel/logging.h>
#include <cpu/idt.h>
#include <stdint.h>
#include <libc/string.h>
#include <kernel/multiboot.h>
#include <drivers/framebuffer.h>
#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000

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

void handle_multiboot(uintptr_t addr) {
    // mb_basic_meminfo_t *basic_meminfo = (mb_basic_meminfo_t*)(multiboot_basic_meminfo + KERNEL_VIRTUAL_ADDR);
    // mb_mmap_t *mmap = (mb_mmap_t*)(multiboot_mmap_data + KERNEL_VIRTUAL_ADDR);
    mb_framebuffer_data_t *framebuffer = (mb_framebuffer_data_t*)(multiboot_framebuffer_data + KERNEL_VIRTUAL_ADDR);
    init_framebuffer(framebuffer);
}

void kernel_start(uintptr_t addr, uint64_t magic) {
    init_qemu();
    log(Info, "KERNEL", "Loaded into kernel");
    init_idt();
    handle_multiboot(addr);
    log(Info, "KERNEL", "Initialized framebuffer");
    while (1);
}