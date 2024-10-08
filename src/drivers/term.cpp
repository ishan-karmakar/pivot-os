#include <drivers/term.hpp>
#include <unifont.h>
#include <limine.h>
#include <limine_terminal/term.h>
#include <lib/modules.hpp>
#include <io/stdio.hpp>
#include <lib/logger.hpp>
#include <drivers/fs/devtmpfs.hpp>
using namespace term;

__attribute__((section(".requests")))
static limine_framebuffer_request fb_request = { LIMINE_FRAMEBUFFER_REQUEST, 2, nullptr };

struct fb_cdev_t : devtmpfs::cdev_t {
    fb_cdev_t(void *address, std::size_t size) : fb{static_cast<char*>(address)}, size{size} {}
    ~fb_cdev_t() = default;
    
    ssize_t read(void *buffer, size_t size, off_t off) {
        size = std::min(this->size - off, size);
        memcpy(fb + off, buffer, size);
        return size;
    }

    ssize_t write(void *buffer, size_t size, off_t off) {
        size = std::min(this->size - off, size);
        memcpy(buffer, fb + off, size);
        return size;
    }

private:
    char *fb;
    std::size_t size;
};

struct tty_cdev_t : devtmpfs::cdev_t {};

std::vector<term_t*> terms{fb_request.response->framebuffer_count};

class term_writer {
public:
    void append(char c) {
        for (const auto& t : terms)
            term_write(t, &c, 1);
    }

    void append(std::string_view s) {
        for (const auto& t : terms)
            term_write(t, s.data(), s.size());
    }

    void append(const char *s) { append(std::string_view{s}); }
};

static term_writer *writer;

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

    writer = new term_writer;
    io::print = [](const char *f, frg::va_struct *args) {
        return frg::printf_format(io::char_printer{*writer, args}, f, args);
    };

    logger::info("TERM", "Initialized terminal");
}

void term::register_devs() {
    for (std::size_t i = 0; i < fb_request.response->framebuffer_count; i++) {
        auto fb = fb_request.response->framebuffers[i];
        devtmpfs::add_dev(std::string{"fb"} + std::to_string(i), new fb_cdev_t{fb->address, fb->pitch * fb->height}, S_IFCHR | S_IRWXU);
    }
}

void term::clear() {
    for (auto& t : terms)
        t->clear(t, true);
}
