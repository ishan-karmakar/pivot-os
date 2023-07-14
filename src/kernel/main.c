#include <kernel/logging.h>
#include <cpu/idt.h>
#include <stdint.h>
#include <libc/string.h>
#include <kernel/multiboot.h>
#include <drivers/framebuffer.h>
#include <cpu/mem.h>

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
    mb_basic_meminfo_t *basic_meminfo = (mb_basic_meminfo_t*)(multiboot_basic_meminfo + KERNEL_VIRTUAL_ADDR);
    mb_mmap_t *mmap = (mb_mmap_t*)(multiboot_mmap_data + KERNEL_VIRTUAL_ADDR);
    mb_framebuffer_data_t *framebuffer = (mb_framebuffer_data_t*)(multiboot_framebuffer_data + KERNEL_VIRTUAL_ADDR);
    mb_acpi_t *acpi_tag = (mb_acpi_t*)(multiboot_acpi_info + KERNEL_VIRTUAL_ADDR);
    init_framebuffer(framebuffer);
    log(Info, "KERNEL", "Initialized framebuffer");
    log(Verbose, "KERNEL", "Basic mem info type: %x", basic_meminfo->type);
    log(Verbose, "KERNEL", "Memory lower: %u, Upper: %u", basic_meminfo->mem_lower, basic_meminfo->mem_upper);
    size_t memory_size = (basic_meminfo->mem_upper + 1024) * 1024;
    init_mem(addr, *(uint32_t*) addr, memory_size);
    // uint32_t num_entries = (mmap->size - sizeof(mb_mmap_t)) / mmap->entry_size;
    // mb_mmap_entry_t *best_region = NULL;
    // for (uint32_t i = 0; i < num_entries; i++) {
    //     log(Verbose, "KERNEL", "[%d] Address: %x, Size: %x, Type: %x", i, mmap->entries[i].addr, mmap->entries[i].len, mmap->entries[i].type);
    //     if (best_region == NULL || 
    //         (mmap->entries[i].type == 0x1 && mmap->entries[i].len > best_region->len))
    //         best_region = &mmap->entries[i];
    // }
}

void kernel_start(uintptr_t addr, uint64_t magic) {
    init_qemu();
    log(Info, "KERNEL", "Loaded into kernel");
    init_idt();
    log(Info, "KERNEL", "Initialized IDT");
    handle_multiboot(addr);
    while (1);
}