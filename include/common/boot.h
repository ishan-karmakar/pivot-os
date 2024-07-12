#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct mmap_desc {
    uint32_t type;
    uint32_t pad;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t count;
    uint64_t attributes;
} mmap_desc_t;

typedef struct boot_info {
    // ACPI
    uintptr_t rsdp;
    bool xsdt;

    // MEM
    uint64_t *pml4;
    uintptr_t stack; // Stack start
    mmap_desc_t *mmap;
    uint64_t mmap_size, desc_size;
    size_t mem_pages;
} boot_info_t;