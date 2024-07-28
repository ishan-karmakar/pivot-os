#pragma once
#include <cstdint>
#include <io/stdio.hpp>

namespace mem {
    class PTMapper;
}

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
        Framebuffer(boot_info*, mem::PTMapper&, uint32_t = 0xFFFFFFFF, uint32_t = 0);
        void clear() override;
        void set_pos(coord_t) override;
        coord_t get_pos() override;
        coord_t get_lims() override;
    
    private:
        void write_char(char) override;
        void find_last();
        size_t get_off();
        void putchar(char);

        const font * const font;
        char * const buffer;
        uint32_t hres, vres, pps;
        uint32_t num_cols, num_rows;
        uint32_t fg, bg;
        uint32_t x, y;

        static constexpr int BPP = 4;
        static constexpr int TAB_SIZE = 4;
    };
}