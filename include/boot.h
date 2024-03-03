#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct mmap_descriptor {
    uint32_t type;
    uint32_t pad;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t count;
    uint64_t attributes;
} mmap_descriptor_t;

typedef struct framebuffer_info {
    uintptr_t pointer;
    uint32_t horizontal_res;
    uint32_t vertical_res;
    uint32_t pixels_per_scanline;
    uint8_t bpp;
} framebuffer_info_t;

typedef struct kernel_entry {
    uintptr_t vaddr;
    uintptr_t paddr;
    size_t num_pages;
} kernel_entry_t;

typedef struct mem_info {
    mmap_descriptor_t *mmap;
    uint64_t mmap_size;
    uint64_t mmap_descriptor_size;
    size_t num_kernel_entries;
    kernel_entry_t *kernel_entries;
    uint64_t *pml4;
    uint64_t *bitmap;
    size_t mem_pages;
} mem_info_t;

typedef struct boot_info {
    framebuffer_info_t fb_info;
    mem_info_t mem_info;
    uintptr_t sdt_address;
    bool xsdt;
} boot_info_t;