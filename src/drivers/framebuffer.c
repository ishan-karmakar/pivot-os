#include <stddef.h>
#include <drivers/framebuffer.h>
#include <kernel/multiboot.h>
#include <libc/string.h>
#include <kernel/logging.h>
#include <mem/mem.h>
#include <stdarg.h>

framebuffer_info_t fbinfo;
psf_font_t *loaded_font;
screen_info_t screen_info = { 0, 0, 0xFFFFFFFF, 0, 0, 0 };
static char buf[33];
int FRAMEBUFFER_INITIALIZED = 0;

inline static uint8_t *get_glyph(uint8_t sym_num) {
    return (uint8_t*) loaded_font + loaded_font->headersize + sym_num * loaded_font->bytesperglyph;
}

static void map_framebuffer(void) {
    log(Verbose, "FB", "Framebuffer memory size: %x", fbinfo.memory_size);
    uint32_t num_pages = fbinfo.memory_size / PAGE_SIZE;
    if (fbinfo.memory_size % PAGE_SIZE)
        num_pages++;
    for (uint32_t i = 0; i < num_pages; i++) {
        map_addr(fbinfo.phys_addr + i * PAGE_SIZE, FRAMEBUFFER_START + i * PAGE_SIZE, WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit_addr(fbinfo.phys_addr + i * PAGE_SIZE);
    }
}

extern void putchar(char sym, screen_info_t *si) {
    uint8_t *glyph = get_glyph(sym);
    size_t bytes_per_line = (loaded_font->width + 7) / 8;
    size_t offset = (si->y * loaded_font->height * fbinfo.pitch) +
                    (si->x * loaded_font->width * sizeof(uint32_t));
    for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
        cy++, glyph += bytes_per_line, offset += fbinfo.pitch, line = offset)
        for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += sizeof(fbinfo.bpp / 8))
            *((uint32_t*) (fbinfo.address + line)) = glyph[cx / 8] & (0x80 >> (cx & 7)) ? si->fg : si->bg;
}

static void find_last_char(screen_info_t *si) {
    if (si->x == 0) {
        if (si->y == 0)
            return;
        si->x = si->num_cols - 1;
        si->y--;
    } else
        si->x--;
    while (1) {
        size_t offset = (si->y * loaded_font->height * fbinfo.pitch) +
                        (si->x * loaded_font->width * sizeof(uint32_t));
        for (uint32_t cy = 0, line = offset; cy < loaded_font->height;
            cy++, offset += fbinfo.pitch, line = offset)
            for (uint32_t cx = 0; cx < loaded_font->width; cx++, line += sizeof(fbinfo.bpp / 8))
                if (*((uint32_t*) (fbinfo.address + line)) != si->bg)
                    return;
        if (si->x == 0)
            return;
        else
            si->x--;
    }
}

void clear_screen(screen_info_t *si) {
    for (size_t i = 0; i < fbinfo.memory_size / 4; i++)
        *((uint32_t*) fbinfo.address + i) = si->bg;
    si->x = 0;
    si->y = 0;
}

void print_char(char c) {
    switch (c) {
    case '\n':
        screen_info.x = 0;
        screen_info.y++;
        break;
    
    case '\b':
        find_last_char(&screen_info);
        putchar(' ', &screen_info);
        break;

    case '\t':
        for (int i = 0; i < TAB_SIZE; i++)
            print_char(' ');
        break;

    default:
        putchar(c, &screen_info);
        screen_info.x++;
        if (screen_info.x >= screen_info.num_cols) {
            screen_info.x = 0;
            screen_info.y++;
        }
    }
    // Check if y is past bounds
    // If it is, then scroll
    if (screen_info.y >= screen_info.num_rows) {
        // TODO: Get scrolling working reliably, for now just clearing screen
        // uint32_t row_size = loaded_font->height * fbinfo.width * sizeof(uint32_t);
        // for (uint32_t i = 1; i < screen_info.num_rows; i++)
        //     memcpy(fbinfo.address + (i - 1) * row_size, fbinfo.address + i * row_size, row_size);
        // memset(fbinfo.address + (screen_info.num_rows - 1) * row_size, screen_info.bg, row_size);
        // screen_info.y = screen_info.num_rows - 1;
        clear_screen(&screen_info);
    }
}

inline static void print_string(char *str, char_printer_t char_printer) {
    for (; *str != '\0'; str++)
        char_printer(*str);
}

static void print_num(int64_t num, char_printer_t char_printer) {
    itoa(num, buf, 21, 10);
    print_string(buf, char_printer);
}

inline static void print_unum(uint64_t num, char_printer_t char_printer) {
    print_string(ultoa(num, buf, 10), char_printer);
}

static void print_hex(uint64_t num, char_printer_t char_printer) {
    print_string("0x", char_printer);
    print_string(ultoa(num, buf, 16), char_printer);
}

void voutf(const char *c, char_printer_t char_printer, va_list args) {
    for (; *c != '\0'; c++)
        if (*c != '%')
            char_printer(*c);
        else
            switch (*++c) {
                case 's':
                    print_string(va_arg(args, char*), char_printer);
                    break;
                case 'c':
                    char_printer(va_arg(args, int));
                    break;
                case 'd':
                    print_num(va_arg(args, int64_t), char_printer);
                    break;
                case 'u':
                    print_unum(va_arg(args, uint64_t), char_printer);
                    break;
                case 'x':
                    print_hex(va_arg(args, uint64_t), char_printer);
                    break;
            }
}

void outf(const char *format, char_printer_t char_printer, ...) {
    va_list args;
    va_start(args, char_printer);
    voutf(format, char_printer, args);
    va_end(args);
}

void vprintf(const char *format, va_list args) {
    voutf(format, print_char, args);
}

void printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    voutf(format, print_char, args);
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
    FRAMEBUFFER_INITIALIZED = 1;
    log(Info, "FB", "Initialized framebuffer");
}