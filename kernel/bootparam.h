#pragma once
#include <stdint.h>

typedef struct {
    uint8_t    *framebuffer;
    uint32_t    width;
    uint32_t    height;
    uint32_t    pitch;
    int             argc;
    char            **argv;
} bootparam_t;

extern bootparam_t* bootp;
