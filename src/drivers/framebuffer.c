#include <stddef.h>
#include <drivers/framebuffer.h>
#include <kernel/multiboot.h>
#include <libc/string.h>
#include <kernel/logging.h>
#define PML4_ENTRY(address)((address>>39) & 0x1ff)
#define PDPR_ENTRY(address)((address>>30) & 0x1ff)
#define PD_ENTRY(address)((address>>21) & 0x1ff)
#define PAGE_SIZE 0x200000
#define PRESENT_BIT 1
#define WRITE_BIT 0b10
#define HUGEPAGE_BIT 0b10000000
#define PAGE_TABLE_ENTRY HUGEPAGE_BIT | WRITE_BIT | PRESENT_BIT

extern uint64_t p2_table[512];
framebuffer_info_t fbinfo;
psf_font_t *loaded_font;
screen_info_t screen_info = { 0, 0, 0xFFFFFFFF, 0, 0, 0 };

inline uint8_t *get_glyph(uint8_t sym_num) {
    return (uint8_t*) loaded_font + loaded_font->headersize + sym_num * loaded_font->bytesperglyph;
}

void map_framebuffer(void) {
    uint32_t pd = PD_ENTRY(_FRAMEBUFFER_MEM_START);
    uint32_t num_pages = fbinfo.memory_size / PAGE_SIZE;
    if (fbinfo.memory_size % PAGE_SIZE)
        num_pages++;
    for (uint32_t i = 0; i < num_pages; i++)
        p2_table[pd + i] = (fbinfo.phys_addr + i * PAGE_SIZE) | PAGE_TABLE_ENTRY;
}

void putchar(char sym, screen_info_t *si) {
    uint8_t *glyph = get_glyph(sym);
    size_t bytes_per_line = (loaded_font->width + 7) / 8;
    size_t offset = (si->y * loaded_font->height * fbinfo.pitch) +
                    (si->x * loaded_font->width * sizeof(uint32_t));
    for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
        cy++, glyph += bytes_per_line, offset += fbinfo.pitch, line = offset)
        for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += sizeof(fbinfo.bpp / 8))
            *((uint32_t*) (fbinfo.address + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? si->fg : si->bg;
}

void print_string(char *str) {
    for (; *str != '\0'; str++) {
        if (*str == '\n') {
            screen_info.x = 0;
            screen_info.y++;
        } else {
            putchar(*str, &screen_info);
            screen_info.x++;
            if (screen_info.x >= screen_info.num_cols) {
                screen_info.x = 0;
                screen_info.y++;
            }
        }
        // Check if y is past bounds
        // If it is, then scroll
        if (screen_info.y >= screen_info.num_rows) {
            uint32_t row_size = loaded_font->height * fbinfo.width * sizeof(uint32_t);
            for (uint32_t i = 1; i < screen_info.num_rows; i++)
                memcpy(fbinfo.address + (i - 1) * row_size, fbinfo.address + i * row_size, row_size);
            memset(fbinfo.address + (screen_info.num_rows - 1) * row_size, screen_info.bg, row_size);
        }
    }
}

void init_framebuffer(mb_framebuffer_data_t *fbdata) {
    loaded_font = &_binary_fonts_default_psf_start;
    fbinfo.address = (uint8_t*) _FRAMEBUFFER_MEM_START;
    fbinfo.bpp = fbdata->framebuffer_bpp;
    fbinfo.height = fbdata->framebuffer_height;
    fbinfo.pitch = fbdata->framebuffer_pitch;
    fbinfo.width = fbdata->framebuffer_width;
    fbinfo.memory_size = fbinfo.height * fbinfo.pitch;
    fbinfo.phys_addr = fbdata->framebuffer_addr;
    screen_info.num_cols = fbinfo.width / loaded_font->width;
    screen_info.num_rows = fbinfo.height / loaded_font->height;
    map_framebuffer();
    qemu_write_string("Initialized framebuffer\n");
}