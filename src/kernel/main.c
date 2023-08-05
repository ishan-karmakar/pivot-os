#include <kernel/logging.h>
#include <cpu/idt.h>
#include <stdint.h>
#include <libc/string.h>
#include <kernel/multiboot.h>
#include <drivers/framebuffer.h>
#include <mem/mem.h>
#include <kernel/acpi.h>
#include <cpu/lapic.h>

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
    uint32_t mbi_size = *(uint32_t*) (addr + KERNEL_VIRTUAL_ADDR);
    mb_basic_meminfo_t *basic_meminfo = (mb_basic_meminfo_t*)(multiboot_basic_meminfo + KERNEL_VIRTUAL_ADDR);
    mb_mmap_t *mmap = (mb_mmap_t*)(multiboot_mmap_data + KERNEL_VIRTUAL_ADDR);

    log(Verbose, "KERNEL", "Memory lower: %u, Upper: %u", basic_meminfo->mem_lower, basic_meminfo->mem_upper);
    size_t memory_size = (basic_meminfo->mem_upper + 1024) * 1024;
    mmap_parse(mmap);
    init_mem(addr, mbi_size, memory_size);

    mb_framebuffer_data_t *framebuffer = (mb_framebuffer_data_t*)(multiboot_framebuffer_data + KERNEL_VIRTUAL_ADDR);
    init_framebuffer(framebuffer);

    mb_tag_t *acpi_tag = (mb_tag_t*)(multiboot_acpi_info + KERNEL_VIRTUAL_ADDR);
    init_acpi(acpi_tag);
}

void kernel_start(uintptr_t addr, uint64_t magic __attribute__((unused))) {
    init_qemu();
    log(Info, "KERNEL", "Loaded into kernel");
    init_idt();
    log(Info, "KERNEL", "Initialized IDT");
    handle_multiboot(addr);

    if (magic == 0x36d76289)
        log(Info, "KERNEL", "Multiboot magic number verified");
    else {
        log(Error, "KERNEL", "Failed to verify magic number");
        hcf();
    }

    init_apic();
    while (1);
}