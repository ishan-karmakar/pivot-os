#pragma once
#include <stdint.h>

typedef struct {
    uint64_t physical_start;
    uint64_t num_pages;
} mem_region_t;

typedef struct {
    uint8_t    *framebuffer;
    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    mem_region_t mem_region;
} bootparam_t;

extern bootparam_t* bootp;
