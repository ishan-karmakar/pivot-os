#pragma once
#include <stdint.h>
#define MAX_GDT_ENTRIES 10

typedef struct gdtr {
    uint16_t size;
    uintptr_t addr;
} __attribute__((packed)) gdtr_t;

typedef union {
    struct {
        uint16_t limit0;
        uint16_t base0;
        uint8_t base1;
        uint8_t access_byte;
        uint8_t flags; // Lower 4 bits is limit, Higher 4 bits is flags
        uint8_t base2;
    } fields;
    uint64_t raw;
} __attribute__((packed)) gdt_desc_t;

extern uint16_t gdt_entries;
extern gdtr_t gdtr;

/// @brief Loads the GDT
void init_gdt(void);
void set_gdt_desc(uint16_t, uint64_t);
extern void load_gdt(uintptr_t);