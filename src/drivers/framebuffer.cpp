#include <kernel.hpp>
#include <drivers/framebuffer.hpp>
#include <lib/logger.hpp>
#include <lib/modules.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <mem/heap.hpp>
#include <limine.h>
#include <limine_terminal/term.h>
#include <unifont.h>
using namespace fb;

__attribute__((section(".requests")))
static limine_framebuffer_request fb_request = { LIMINE_FRAMEBUFFER_REQUEST, 2, nullptr };

// TODO: Support multiple framebuffers
// TODO: Support BPP != 32

void fb::init() {
    auto font_file = mod::find("font");

    font_t font = {
        reinterpret_cast<uintptr_t>(font_file->address),
        UNIFONT_WIDTH,
        UNIFONT_HEIGHT,
        TERM_FONT_SPACING,
        TERM_FONT_SCALE_X,
        TERM_FONT_SCALE_Y
    };

    style_t style = {
        TERM_ANSI_COLOURS,
        TERM_ANSI_BRIGHT_COLOURS,
        TERM_BACKGROUND,
        TERM_FOREGROUND,
        TERM_BACKGROUND_BRIGHT,
        TERM_FOREGROUND_BRIGHT,
        TERM_MARGIN,
        TERM_MARGIN_GRADIENT
    };

    background_t back = {
        nullptr,
        IMAGE_TILED,
        TERM_BACKDROP
    };

    std::vector<term_t*> terms{fb_request.response->framebuffer_count};
    for (std::size_t i = 0; i < fb_request.response->framebuffer_count; i++) {
        auto framebuffer = fb_request.response->framebuffers[i];

        terms[i] = term_init(
            {
                reinterpret_cast<uintptr_t>(framebuffer->address),
                framebuffer->width,
                framebuffer->height,
                framebuffer->pitch,
                framebuffer->red_mask_size,
                framebuffer->red_mask_shift,
                framebuffer->green_mask_size,
                framebuffer->green_mask_shift,
                framebuffer->blue_mask_size,
                framebuffer->blue_mask_shift
            },
            font,
            style,
            back
        );
        terms[i]->clear(terms[i], true);
    }
}