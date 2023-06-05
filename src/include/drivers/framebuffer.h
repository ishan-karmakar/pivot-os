#pragma once
#include <stdint.h>
#include <kernel/multiboot.h>

#define _FRAMEBUFFER_MEM_START 0xffffffffbd000000

typedef struct {
    uint8_t *address;
    uint8_t bpp;
    uint32_t pitch;
    uint32_t memory_size;
    uint32_t width;
    uint32_t height;
    uintptr_t phys_addr;
} framebuffer_info_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} psf_font_t;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t fg;
    uint32_t bg;
    uint32_t num_rows;
    uint32_t num_cols;
} screen_info_t;

extern psf_font_t _binary_fonts_default_psf_start;
extern char _binary_fonts_default_psf_size;
extern char _binary_fonts_default_psf_end;

void init_framebuffer(mb_framebuffer_data_t*);