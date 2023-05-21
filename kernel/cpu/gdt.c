#include <stdint.h>
#include "gdt.h"
#include "log.h"

#define GDT_ENTRIES 5

#define PRESENT (1 << 7)
#define CODE_DATA (1 << 4)
#define READ_WRITE (1 << 1)
#define EXECUTABLE (1 << 3)
#define BITS_64 (1 << 1)

#define KERNEL_CODE (PRESENT | CODE_DATA | READ_WRITE | EXECUTABLE)
#define KERNEL_DATA (PRESENT | CODE_DATA | READ_WRITE)
#define TSS (PRESENT | 0x09)

#pragma pack(1)
struct tss_t {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
};

struct gdtr_t {
    uint16_t size;
    uint64_t base;
};

struct gdt_entry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
};

struct tss_entry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_flags;
    uint8_t base_mid2;
    uint32_t base_high;
    uint32_t rsv;
};

static struct {
    struct gdt_entry_t null;
    struct gdt_entry_t kernel_code;
    struct gdt_entry_t kernel_data;
    struct tss_entry_t tss;
} gdt_table = {
    { 0 }, // Null descriptor
    { 0, 0, 0, KERNEL_CODE, BITS_64 << 4, 0 },
    { 0, 0, 0, KERNEL_DATA, 0, 0 },
    { 0 } // Initialize later
};
#pragma pack()

static struct gdtr_t gdtr;
static struct tss_t tss;

void load_gdt(void) {
    uint64_t tss_addr = (uintptr_t) &tss;
    uint64_t tss_size = sizeof(tss) - 1;
    gdt_table.tss = (struct tss_entry_t) {
        tss_size & 0xFFFF,
        tss_addr & 0xFFFF,
        (tss_addr >> 16) & 0xFFFF,
        TSS,
        (tss_size >> 16) & 0xF,
        (tss_addr >> 24) & 0xFF,
        (tss_addr >> 32) & 0xFFFF,
        0
    };

    gdtr.size = sizeof(gdt_table) - 1;
    gdtr.base = (uintptr_t) &gdt_table;
    asm volatile ("lgdt %0" : : "m" (gdtr));

    asm volatile (
        "mov %0, %%ax\n"
        "ltr %%ax"
        : : "i" (0x18) : "memory"
    );

    asm volatile (
        "pushq %0\n"
        "pushq $1f\n"
        "lretq\n"
        "1:\n"
        : : "i" (0x8) : "memory"
    );

    asm volatile (
        "mov %0, %%ds\n"
        "mov %0, %%es\n"
        "mov %0, %%fs\n"
        "mov %0, %%gs\n"
        "mov %0, %%ss\n"
        : : "r" (0x10) : "memory"
    );
    log(Info, "GDT", "Initialized GDT");
}
