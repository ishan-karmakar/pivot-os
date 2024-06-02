#include <drivers/framebuffer.h>
#include <mem/pmm.h>
#include <sys.h>
#include <io/stdio.h>
#include <kernel/logging.h>

extern char _binary_fonts_default_psf_start;
psf_font_t *loaded_font;
framebuffer_info_t *fb_info;
char *fb_buffer;
size_t screen_num_cols, screen_num_rows;
size_t screen_x = 0, screen_y = 0;
uint32_t screen_fg = 0xFFFFFFFF, screen_bg = 0;

static uint8_t *get_glyph(uint8_t sym) {
    return (uint8_t*) loaded_font + loaded_font->header_size + sym * loaded_font->bytes_per_glyph;
}

static size_t get_offset(void) {
    return (screen_y * loaded_font->height * (fb_info->bpp * fb_info->pixels_per_scanline)) +
            (screen_x * loaded_font->width * fb_info->bpp);
}

static void find_last_char(void) {
    if (screen_x == 0) {
        if (screen_y == 0)
            return;
        screen_x = screen_num_cols - 1;
        screen_y--;
    } else
        screen_x--;
    
    while (1) {
        size_t offset = get_offset();
        for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
            cy++, offset += (fb_info->bpp * fb_info->pixels_per_scanline), line = offset)
            for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += fb_info->bpp)
                if (*((uint32_t*) (fb_buffer + line)) != screen_bg)
                    return;
        
        if (screen_x == 0)
            return;
        screen_x--;
    }
}

void putchar(char sym) {
    uint8_t *glyph = get_glyph(sym);
    size_t bytes_per_line = (loaded_font->width + 7) / 8;
    size_t offset = get_offset();
    for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
        cy++, glyph += bytes_per_line, offset += fb_info->bpp * fb_info->pixels_per_scanline, line = offset)
        for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += fb_info->bpp)
            *((uint32_t*) (fb_buffer + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? screen_fg : screen_bg;
}

void fb_print_char(char c) {
    switch (c) {
    case '\n':
        screen_x = 0;
        screen_y++;
        break;

    case '\b':
        find_last_char();
        putchar(' ');
        break;

    case '\t':
        for (int i = 0; i < 4; i++)
            fb_print_char(' ');
        break;
    
    default:
        putchar(c);
        screen_x++;
        if (screen_x >= screen_num_cols) {
            screen_x = 0;
            screen_y++;
        }
    }
    // TODO: Scroll screen intead of clearing
    if (screen_y >= screen_num_rows)
        clear_screen();
}

void clear_screen(void) {
    for (size_t i = 0; i < (fb_info->pixels_per_scanline * fb_info->vertical_res); i++)
        *((uint32_t*) fb_buffer + i) = screen_bg;
    screen_x = screen_y = 0;
}

void map_framebuffer(page_table_t p4_tbl, size_t flags) {
    size_t fb_size = fb_info->bpp * fb_info->pixels_per_scanline * fb_info->vertical_res;
    map_range(fb_info->pointer, fb_info->pointer, SIZE_TO_PAGES(fb_size), flags, p4_tbl);
    pmm_set_area(fb_info->pointer, SIZE_TO_PAGES(fb_size));
}

void init_framebuffer(framebuffer_info_t *fbinfo) {
    loaded_font = (psf_font_t*) &_binary_fonts_default_psf_start;
    fb_info = fbinfo;
    screen_num_cols = fb_info->horizontal_res / loaded_font->width;
    screen_num_rows = fb_info->vertical_res / loaded_font->height;
    fb_buffer = (char*) fb_info->pointer;
    map_framebuffer(NULL, KERNEL_PT_ENTRY);
    char_printer = fb_print_char;
    clear_screen();
    log(Info, "FB", "Initialized framebuffer");
}
