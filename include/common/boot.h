#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef uint64_t* pg_tbl_t;

struct mmap_desc {
    uint32_t type;
    uint32_t pad;
    uintptr_t phys;
    uintptr_t virt;
    uint64_t count;
    uint64_t attributes;
};

struct boot_info {
    // ACPI
    uintptr_t rsdp;

    // MEM
    pg_tbl_t pml4;
    uintptr_t stack; // Stack start
    struct mmap_desc *mmap;
    size_t mmap_entries;
    size_t mem_pages;
    size_t desc_size;

    // FRAMEBUFFER
    uintptr_t fb_buf;
    uint32_t hres;
    uint32_t vres;
    uint32_t pps;
};