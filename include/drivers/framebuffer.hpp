#pragma once
#include <cstdint>
#include <io/stdio.hpp>

struct limine_framebuffer;

namespace fb {
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

    class framebuffer : public io::owriter {
    public:
        framebuffer(limine_framebuffer*, uint32_t = 0xFFFFFFFF, uint32_t = 0);
        void clear() override;
        void set_pos(coord_t) override;
        coord_t get_pos() override;
        coord_t get_constraints() override;
    
    private:
        void append(char) override;
        void find_last();
        std::size_t get_off();
        void putchar(char);

        char *buffer;
        limine_framebuffer *info;
        uint64_t num_cols, num_rows;
        uint32_t fg, bg;
        uint32_t x, y;

        static constexpr int BPP = 4;
        static constexpr int TAB_SIZE = 4;
    };
    
    void init();

    extern const struct font *font;
    extern framebuffer *kfb;
}