#include <drivers/term.hpp>
#include <unifont.h>
#include <limine.h>
#include <limine_terminal/term.h>
#include <lib/modules.hpp>
#include <io/stdio.hpp>
#include <lib/logger.hpp>
using namespace term;

__attribute__((section(".requests")))
static limine_framebuffer_request fb_request = { LIMINE_FRAMEBUFFER_REQUEST, 2, nullptr };

std::vector<term_t*> term::terms{fb_request.response->framebuffer_count};

struct term_writer : public io::owriter {
    void append(char c) override {
        for (const auto& t : terms)
            term_write(t, &c, 1);
    }

    void append(std::string_view s) override {
        for (const auto& t : terms)
            term_write(t, s.data(), s.size());
    }
};

void term::init() {
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
        TERM_FOREGROUND_BRIGHT,
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
        terms[i]->cursor_enabled = false;
    }
    clear();
    io::writer = new term_writer;
    logger::info("TERM", "Initialized terminal");
}
