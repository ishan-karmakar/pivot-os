#include <kernel.hpp>
#include <drivers/framebuffer.hpp>
#include <lib/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <mem/heap.hpp>
#include <limine.h>
using namespace fb;

extern char _binary_default_psf_start;
framebuffer *fb::kfb = nullptr;
const struct fb::font *fb::font = reinterpret_cast<struct font*>(&_binary_default_psf_start);

__attribute__((section(".requests")))
static volatile limine_framebuffer_request fb_request = { LIMINE_FRAMEBUFFER_REQUEST, 2, nullptr };

extern volatile limine_memmap_request mmap_request;

// TODO: Support multiple framebuffers
// TODO: Support BPP != 32

void fb::init() {
    kfb = new framebuffer{fb_request.response->framebuffers[0]};
    io::writer = kfb;
    kfb->clear();
    logger::info("FB[INIT]", "Initialized kernel framebuffer");
}

// Create new instance of framebuffer - Doesn't clear screen automatically
framebuffer::framebuffer(limine_framebuffer *info, uint32_t fg, uint32_t bg) :
    buffer{reinterpret_cast<char*>(info->address)},
    info{info}, num_cols{info->width / font->width}, num_rows{info->height / font->height}, fg{fg}, bg{bg}
{}

void framebuffer::append(char c) {
    if (y >= num_rows)
        clear();
    switch (c) {
    case '\n':
        x = 0;
        y++;
        break;

    case '\b': {
        find_last();
        uint32_t tmp_x = x, tmp_y = y;
        append(' ');
        x = tmp_x;
        y = tmp_y;
        break;
    }
    case '\t':
        for (int i = 0; i < TAB_SIZE; i++)
            append(' ');
        break;
    
    default:    
        putchar(c);
        if (++x >= num_cols) {
            x = 0;
            y++;
        }
    }
}

void framebuffer::clear() {
    for (std::size_t line = 0; line < info->height; line++)
        for (std::size_t i = 0; i < info->width; i++)
            reinterpret_cast<uint32_t*>(buffer + line * info->pitch)[i] = bg;
    x = y = 0;
}

void framebuffer::putchar(char c) {
    const uint8_t * glyph = reinterpret_cast<const uint8_t*>(font) + font->header_size + c * font->bpg;
    std::size_t bpl = div_ceil(font->width, 8);
    std::size_t off = get_off();
    for (uint32_t cy = 0, line = off; cy < font->height;
        cy++, glyph += bpl, off += info->pitch, line = off)
        for (uint32_t cx = 0; cx < font->width; cx++, line += BPP)
            *((uint32_t*) (buffer + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? fg : bg;
}

void framebuffer::find_last() {
    if (!x) {
        if (!y)
            return;
        x = num_cols - 1;
        y--;
    } else
        x--;
    
    while (true) {
        std::size_t off = get_off();
        for (uint32_t cy = 0, line = off; cy < font->height; cy++, off += BPP * info->pitch, line = off)
            for (uint32_t cx = 0; cx < font->width; cx++, line += BPP)
                if (*((uint32_t*) (buffer + line)) != bg)
                    return;

        if (x == 0)
            return;
        x--;
    }
}

void framebuffer::set_pos(coord_t pos) {
    x = pos.first;
    y = pos.second;
}

framebuffer::coord_t framebuffer::get_pos() {
    return { x, y };
}

framebuffer::coord_t framebuffer::get_constraints() {
    return { num_cols, num_rows };
}

std::size_t framebuffer::get_off() {
    return (y * font->height * info->pitch) + (x * font->width * BPP);
}