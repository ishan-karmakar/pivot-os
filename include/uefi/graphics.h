#pragma once
#include <uefi.h>

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} efi_graphics_pixel_format_t;

typedef struct {
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t rsv_mask;
} efi_pixel_bitmask_t;

typedef struct {
    uint32_t version;
    uint32_t horizontal_res;
    uint32_t vertical_res;
    efi_graphics_pixel_format_t pixel_format;
    efi_pixel_bitmask_t pixel_info;
    uint32_t pp_scan_line;
} efi_gop_mode_info_t;

typedef struct {
    uint32_t max_mode;
    uint32_t mode;
    efi_gop_mode_info_t *info;
    size_t info_size;
    uintptr_t fb_base;
    size_t fb_size;
} efi_gop_mode_t;

typedef struct efi_gop {
    efi_status_t (*query_mode)(struct efi_gop*, uint32_t, size_t*, efi_gop_mode_info_t**);
    efi_status_t (*set_mode)(struct efi_gop*, uint32_t);
    uintptr_t blt;
    efi_gop_mode_t *mode;
} efi_gop_t;

efi_status_t init_graphics(void);
