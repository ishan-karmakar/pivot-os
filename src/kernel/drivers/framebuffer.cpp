#include <common.h>
#include <drivers/framebuffer.hpp>
#include <util/logger.h>
#define BPP 4
#define TAB_SIZE 4
using namespace drivers;

extern char _binary_fonts_default_psf_start;

Framebuffer::Framebuffer(boot_info* bi, mem::PTMapper& mapper, mem::PMM& pmm, uint32_t fg, uint32_t bg) :
    font{reinterpret_cast<struct font*>(&_binary_fonts_default_psf_start)}, buffer{reinterpret_cast<char*>(bi->fb_buf)},
    hres{bi->hres}, vres{bi->vres}, pps{bi->pps}, num_cols{hres / font->width}, num_rows{vres / font->height}, fg{fg}, bg{bg}
{
    size_t fb_pages = DIV_CEIL(BPP * pps * vres, PAGE_SIZE);
    mapper.map(bi->fb_buf, bi->fb_buf, fb_pages, KERNEL_PT_ENTRY);
    pmm.set(bi->fb_buf, fb_pages);

    set_global();
    clear();

    log(Info, "FB", "Initialized framebuffer");
}

void Framebuffer::operator<<(char c) {
    switch (c) {
    case '\n':
        x = 0;
        y++;
        break;

    case '\b':
        find_last();
        *this << ' ';
        break;
    
    case '\t':
        for (int i = 0; i < TAB_SIZE; i++)
            *this << ' ';
        break;
    
    default:    
        putchar(c);
        if (++x >= num_cols) {
            x = 0;
            y++;
        }
    }

    if (y >= num_rows)
        clear();
}

void Framebuffer::clear() {
    for (size_t i = 0; i < pps * vres; i++)
        *((uint32_t*) buffer + i) = bg;
    x = y = 0;
}

void Framebuffer::putchar(char c) {
    const uint8_t * glyph = reinterpret_cast<const uint8_t*>(font) + font->header_size + c * font->bpg;
    size_t bpl = DIV_CEIL(font->width, 8);
    size_t off = get_off();
    for (uint32_t cy = 0, line = off; cy < font->height;
        cy++, glyph += bpl, off += BPP * pps, line = off)
        for (uint32_t cx = 0; cx < font->width; cx++, line += BPP)
            *((uint32_t*) (buffer + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? fg : bg;
}

void Framebuffer::find_last() {
    if (!x) {
        if (!y)
            return;
        x = num_cols - 1;
        y--;
    } else
        x--;
    
    while (true) {
        size_t off = get_off();
        for (uint32_t cy = 0, line = off; cy < font->height; cy++, off += BPP * pps, line = off)
            for (uint32_t cx = 0; cx < font->width; cx++, line += BPP)
                if (*((uint32_t*) (buffer + line)) != bg)
                    return;

        if (x == 0)
            return;
        x--;
    }
}

size_t Framebuffer::get_off() {
    return (y * font->height * BPP * pps) + (x * font->width * BPP);
}