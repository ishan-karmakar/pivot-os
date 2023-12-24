#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t type;
    uint32_t pad;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t count;
    uint64_t attributes;
} mmap_descriptor_t;

typedef struct {
    uintptr_t pointer;
    uint32_t horizontal_res;
    uint32_t vertical_res;
    uint32_t pixels_per_scanline;
    uint8_t bpp;
} framebuffer_info_t;

typedef struct {
    mmap_descriptor_t *mmap;
    uint64_t mmap_size;
    uint64_t mmap_descriptor_size;
    framebuffer_info_t fbinfo;
    uint64_t *kernel_entries;
    uint64_t num_kernel_entries;
    uint64_t *pml4;
    uintptr_t sdt_address;
    bool xsdt;
} boot_info_t;