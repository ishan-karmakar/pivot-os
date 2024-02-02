#pragma once
#include <stdint.h>
#include <cpu/tss.h>

#pragma pack(push, default)
#pragma pack(1)

typedef struct {
    uint16_t size;
    uintptr_t addr;
} gdtr_t;

typedef struct {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access_byte;
    uint8_t flags; // Lower 4 bits is limit, Higher 4 bits is flags
    uint8_t base2;
} gdt_desc_t;

#pragma pack(pop)

/// @brief Loads the GDT
void init_gdt(void);
void load_tss(tss_t*);