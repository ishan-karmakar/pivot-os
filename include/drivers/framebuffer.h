#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mem/pmm.h>

extern size_t screen_num_cols, screen_num_rows;
extern size_t screen_x, screen_y;
extern uint32_t screen_fg, screen_bg;

typedef struct psf_font {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t num_glyph;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
} __attribute__((packed)) psf_font_t;

void init_framebuffer(void);
void map_framebuffer(page_table_t, size_t);
void clear_screen(void);