#include <stddef.h>
#include <drivers/framebuffer.h>
#include <kernel/multiboot.h>
#include <libc/string.h>
#include <kernel/logging.h>
#include <mem/mem.h>
#include <stdarg.h>
#include <sys.h>
#define BUF_SIZE 1024

framebuffer_info_t fbinfo;
psf_font_t *loaded_font;
screen_info_t screen_info = { 0, 0, 0xFFFFFFFF, 0, 0, 0 };
static char buf[BUF_SIZE];
size_t buf_pos = 0;
bool FRAMEBUFFER_INITIALIZED = false;

inline static uint8_t *get_glyph(uint8_t sym_num) {
    return (uint8_t*) loaded_font + loaded_font->headersize + sym_num * loaded_font->bytesperglyph;
}

static void map_framebuffer(void) {
    uint32_t num_pages = fbinfo.memory_size / PAGE_SIZE;
    if (fbinfo.memory_size % PAGE_SIZE)
        num_pages++;
    for (uint32_t i = 0; i < num_pages; i++) {
        map_addr(fbinfo.phys_addr + i * PAGE_SIZE, FRAMEBUFFER_START + i * PAGE_SIZE, WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit_addr(fbinfo.phys_addr + i * PAGE_SIZE);
    }
}

extern void putchar(char sym) {
    uint8_t *glyph = get_glyph(sym);
    size_t bytes_per_line = (loaded_font->width + 7) / 8;
    size_t offset = (screen_info.y * loaded_font->height * fbinfo.pitch) +
                    (screen_info.x * loaded_font->width * sizeof(uint32_t));
    for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
        cy++, glyph += bytes_per_line, offset += fbinfo.pitch, line = offset)
        for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += sizeof(fbinfo.bpp / 8))
            *((uint32_t*) (fbinfo.address + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? screen_info.fg : screen_info.bg;
}

static void find_last_char(void) {
    if (screen_info.x == 0) {
        if (screen_info.y == 0)
            return;
        screen_info.x = screen_info.num_cols - 1;
        screen_info.y--;
    } else
        screen_info.x--;
    while (1) {
        size_t offset = (screen_info.y * loaded_font->height * fbinfo.pitch) +
                        (screen_info.x * loaded_font->width * sizeof(uint32_t));
        for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
            cy++, offset += fbinfo.pitch, line = offset)
            for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += sizeof(fbinfo.bpp / 8))
                if (*((uint32_t*) (fbinfo.address + line)) != screen_info.bg)
                    return;
        if (screen_info.x == 0)
            return;
        else
            screen_info.x--;
    }
}

void clear_screen(void) {
    for (size_t i = 0; i < fbinfo.memory_size / 4; i++)
        *((uint32_t*) fbinfo.address + i) = screen_info.bg;
    screen_info.x = 0;
    screen_info.y = 0;
}

void print_char(char c) {
    switch (c) {
    case '\n':
        screen_info.x = 0;
        screen_info.y++;
        break;
    
    case '\b':
        find_last_char();
        putchar(' ');
        break;

    case '\t':
        for (int i = 0; i < TAB_SIZE; i++)
            print_char(' ');
        break;

    default:
        putchar(c);
        screen_info.x++;
        if (screen_info.x >= screen_info.num_cols) {
            screen_info.x = 0;
            screen_info.y++;
        }
    }
    if (screen_info.y >= screen_info.num_rows) {
        // TODO: Get scrolling working reliably, for now just clearing screen
        // uint32_t row_size = loaded_font->height * fbinfo.width * sizeof(uint32_t);
        // for (uint32_t i = 1; i < screen_info.num_rows; i++)
        //     memcpy(fbinfo.address + (i - 1) * row_size, fbinfo.address + i * row_size, row_size);
        // memset(fbinfo.address + (screen_info.num_rows - 1) * row_size, screen_info.bg, row_size);
        // screen_info.y = screen_info.num_rows - 1;
        clear_screen();
    }
}

void flush_screen(void) {
    if (!FRAMEBUFFER_INITIALIZED)
        return;
    for (uint16_t i = 0; i < buf_pos; i++)
        print_char(buf[i]);
    buf_pos = 0;
}

static inline void add_string(char* str) {
    for (; *str != '\0'; str++)
        buf[buf_pos++] = *str;
}

void vprintf(const char *c, va_list args) {
    for (; *c != '\0'; c++) {
        if (*c != '%')
            buf[buf_pos++] = *c;
        else
            switch (*++c) {
                case 's':
                    add_string(va_arg(args, char*));
                    break;
                case 'c':
                    char ch = (char) va_arg(args, int);
                    buf[buf_pos++] = ch;
                    if (ch == '\n')
                        flush_screen();
                    break;
                case 'd':
                    buf_pos += itoa(va_arg(args, int64_t), buf + buf_pos, 21, 10);
                    break;
                case 'u':
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 10);
                    break;
                case 'x':
                    add_string("0x");
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 16);
                    break;
                case 'b':
                    add_string("0b");
                    buf_pos += ultoa(va_arg(args, uint64_t), buf + buf_pos, 2);
            }
        if (buf_pos > (BUF_SIZE - 1)) {
            buf_pos = 0;
            log(Error, true, "FB", "Buffer overflow");
        }
        if (*c == '\n')
            flush_screen();
    }
}

void printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void init_framebuffer(mb_framebuffer_data_t *fbdata) {
    loaded_font = &_binary_fonts_default_psf_start;
    fbinfo.address = (uint8_t*) FRAMEBUFFER_START;
    fbinfo.bpp = fbdata->framebuffer_bpp;
    fbinfo.height = fbdata->framebuffer_height;
    fbinfo.pitch = fbdata->framebuffer_pitch;
    fbinfo.width = fbdata->framebuffer_width;
    fbinfo.memory_size = fbinfo.height * fbinfo.pitch;
    fbinfo.phys_addr = fbdata->framebuffer_addr;
    screen_info.num_cols = fbinfo.width / loaded_font->width;
    screen_info.num_rows = fbinfo.height / loaded_font->height;
    map_framebuffer();
    FRAMEBUFFER_INITIALIZED = true;
    log(Info, true, "FRAMEBUFFER", "Initialized framebuffer");
}