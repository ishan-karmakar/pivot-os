#include <stdint.h>
#include "gdt.h"
#include "log.h"
// Access Byte
#define PRESENT (1 << 7)
#define CODE_DATA (1 << 4)
#define READ_WRITE (1 << 1)
#define EXECUTABLE (1 << 3)

// Flags
#define BITS_64 (1 << 1)

#define GDT_ENTRIES 3

struct __attribute__((packed)) gdtr {
    uint16_t size;
    uint64_t base;
};

struct __attribute__((packed)) gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
};

static struct gdt_entry gdt[GDT_ENTRIES]; // null segment, code + data segment
static struct gdtr gdtr;

static void add_entry(struct gdt_entry* entry, uint8_t access, uint8_t flags) {
    entry->limit_low = 0;
    entry->base_low = 0;
    entry->base_mid = 0;
    entry->access = access;
    entry->flags = (flags << 4);
    entry->base_high = 0;
}

void load_gdt(void) {
    gdt[0] = (struct gdt_entry) { 0 };
    add_entry(&gdt[1], PRESENT | CODE_DATA | READ_WRITE | EXECUTABLE, BITS_64);
    add_entry(&gdt[2], PRESENT | CODE_DATA | READ_WRITE, 0);

    gdtr.size = sizeof(struct gdt_entry) * GDT_ENTRIES - 1;
    gdtr.base = (uintptr_t) &gdt[0];
    asm volatile ("lgdt %0" : : "m" (gdtr));

    asm volatile (
        "pushq %0\n"
        "pushq $1f\n"
        "lretq\n"
        "1:\n"
        : : "i" (0x8) : "memory"
    );

    asm volatile (
        "movl %0, %%ds\n"
        "movl %0, %%es\n"
        "movl %0, %%fs\n"
        "movl %0, %%gs\n"
        "movl %0, %%ss\n"
        : : "r" (0x10) : "memory"
    );
    log(Info, "GDT", "Initialized GDT");
}
