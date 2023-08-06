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
size_t mem_size;

log_level_t min_log_level = Verbose;

__attribute__((noreturn))
void hcf(void) {
    asm volatile ("cli");
    while (1)
        asm volatile ("hlt");
}

typedef struct {
    uint64_t int1;
    uint32_t int2;
    uint16_t int3;
    uint8_t int4;
} __attribute__((__packed__)) test_struct;

void handle_multiboot(uintptr_t addr) {
    uint32_t mbi_size = *(uint32_t*) (addr + KERNEL_VIRTUAL_ADDR);
    mb_basic_meminfo_t *basic_meminfo = (mb_basic_meminfo_t*)(multiboot_basic_meminfo + KERNEL_VIRTUAL_ADDR);
    mb_mmap_t *mmap = (mb_mmap_t*)(multiboot_mmap_data + KERNEL_VIRTUAL_ADDR);

    log(Verbose, "KERNEL", "Memory lower: %x, Upper: %x", basic_meminfo->mem_lower, basic_meminfo->mem_upper);
    mem_size = (basic_meminfo->mem_upper + 1024) * 1024;
    mmap_parse(mmap);
    init_pmm(addr, mbi_size, mem_size);

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
    init_apic(mem_size);
    pmm_map_physical_memory();
    // char *frame = (char*) alloc_frame();
    // char *vaddress = (char*) 0x820000000000;
    // map_addr((uint64_t) frame, (uint64_t) vaddress, WRITE_BIT | PRESENT_BIT);
    // char test = *frame;
    // log(Verbose, "TEST", "%x", (uint64_t) frame);
    init_kheap();
    test_struct *my_struct = (test_struct*) kmalloc(sizeof(test_struct));
    my_struct->int1 = 1;
    my_struct->int2 = 2;
    my_struct->int3 = 3;
    my_struct->int4 = 4;
    log(Verbose, "KERNEL", "Struct size: %d, address: %x", sizeof(*my_struct), (uintptr_t) my_struct);
    while (1);
}