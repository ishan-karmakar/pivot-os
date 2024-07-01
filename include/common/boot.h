#pragma once
#include <stdint.h>

typedef struct mmap_descriptor {
    uint32_t type;
    uint32_t pad;
    uintptr_t physical_start;
    uintptr_t virtual_start;
    uint64_t count;
    uint64_t attributes;
} mmap_desc_t;