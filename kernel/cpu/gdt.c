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
    entry->access = access;
    entry->flags = flags;
}

void load_gdt(void) {
    gdt[0] = (struct gdt_entry) { 0 };
    add_entry(&gdt[1], PRESENT | CODE_DATA | READ_WRITE | EXECUTABLE, BITS_64);
    add_entry(&gdt[2], PRESENT | CODE_DATA | READ_WRITE, 0);

    gdtr.size = sizeof(struct gdt_entry) * GDT_ENTRIES;
    gdtr.base = (uintptr_t) &gdt[0];
    asm volatile (
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "push $0x08\n" 
        "push $setLabel\n"
        // "retfq\n"
        "setLabel: \n"
        : : "m" (gdtr) : "%ax"
    );
    log(Info, "GDT", "Initialized GDT");
}
