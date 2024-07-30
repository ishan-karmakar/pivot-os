#include <kernel.hpp>
#include <drivers/framebuffer.hpp>
#include <util/logger.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <limine.h>
using namespace drivers;

extern char _binary_default_psf_start;
Framebuffer *drivers::kfb = nullptr;
const struct Framebuffer::font *Framebuffer::font = reinterpret_cast<struct font*>(&_binary_default_psf_start);

__attribute__((section(".requests")))
static volatile limine_framebuffer_request fb_request = { LIMINE_FRAMEBUFFER_REQUEST, 2, nullptr };

extern volatile limine_memmap_request mmap_request;

// TODO: Support multiple framebuffers
// TODO: Support BPP != 4

void fb::init() {
    kfb = new Framebuffer{*mem::kmapper, fb_request.response->framebuffers[0]};
    log(Info, "FB", "Initialized kernel framebuffer");
}

Framebuffer::Framebuffer(mem::PTMapper& mapper, limine_framebuffer *info, uint32_t fg, uint32_t bg) :
    buffer{reinterpret_cast<char*>(5)},
    info{info}, num_cols{info->width / font->width}, num_rows{info->height / font->height}, fg{fg}, bg{bg}
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(info->address);
    for (size_t i = 0; i < mmap_request.response->entry_count; i++) {
        auto ent = mmap_request.response->entries[i];
        if (ent->type == LIMINE_MEMMAP_FRAMEBUFFER)
            mapper.map(ent->base, virt_addr(ent->base), KERNEL_PT_ENTRY, DIV_CEIL(ent->length, PAGE_SIZE));
    }
    log(Verbose, "FB", "%p", buffer);
    // clear();
}

void Framebuffer::append(char c) {
    switch (c) {
    case '\n':
        x = 0;
        y++;
        break;

    case '\b':
        find_last();
        append(' ');
        break;
    
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

    if (y >= num_rows)
        clear();
}

void Framebuffer::clear() {
    for (size_t line = 0; line < info->height; line++)
        for (size_t i = 0; i < info->width; i++)
            reinterpret_cast<uint32_t*>(buffer + line * info->pitch)[i] = bg;
    x = y = 0;
}

void Framebuffer::putchar(char c) {
    const uint8_t * glyph = reinterpret_cast<const uint8_t*>(font) + font->header_size + c * font->bpg;
    size_t bpl = DIV_CEIL(font->width, 8);
    size_t off = get_off();
    for (uint32_t cy = 0, line = off; cy < font->height;
        cy++, glyph += bpl, off += BPP * info->pitch, line = off)
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
        for (uint32_t cy = 0, line = off; cy < font->height; cy++, off += BPP * info->pitch, line = off)
            for (uint32_t cx = 0; cx < font->width; cx++, line += BPP)
                if (*((uint32_t*) (buffer + line)) != bg)
                    return;

        if (x == 0)
            return;
        x--;
    }
}

void Framebuffer::set_pos(coord_t pos) {
    x = pos.first;
    y = pos.second;
}

Framebuffer::coord_t Framebuffer::get_pos() {
    return { x, y };
}

Framebuffer::coord_t Framebuffer::get_constraints() {
    return { num_cols, num_rows };
}

size_t Framebuffer::get_off() {
    return (y * font->height * BPP * info->pitch) + (x * font->width * BPP);
}