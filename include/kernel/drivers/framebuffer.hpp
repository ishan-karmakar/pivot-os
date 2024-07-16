#pragma once
#include <io/stdio.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <boot.h>

namespace drivers {
    class Framebuffer : public io::OWriter {
    private:
        struct font {
            uint32_t magic;
            uint32_t version;
            uint32_t header_size;
            uint32_t flags;
            uint32_t num_glyph;
            uint32_t bpg;
            uint32_t height;
            uint32_t width;
        };

    public:
        Framebuffer(boot_info*, mem::PTMapper&, mem::PMM&, uint32_t = 0xFFFFFFFF, uint32_t = 0);
        void operator<<(char) override;
        void clear() override;
    
    private:
        void find_last();
        size_t get_off();
        void putchar(char);

        const font * const font;
        char * const buffer;
        uint32_t hres, vres, pps;
        uint32_t num_cols, num_rows;
        uint32_t fg, bg;
        uint32_t x, y;
    };
}