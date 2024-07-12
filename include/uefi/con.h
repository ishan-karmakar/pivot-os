#pragma once
#include <stdbool.h>
#include <uefi.h>

typedef struct {
    uint16_t scan_code;
    uint16_t unicode_char;
    void *wait_for_key;
} efi_input_key_t;

typedef struct efi_si {
    efi_status_t (*reset)(struct efi_si*, bool);
    efi_status_t (*read_key_stroke)(struct efi_si*, efi_input_key_t*);
} efi_si_t;

typedef struct {
    int32_t max_mode;
    int32_t mode;
    int32_t attribute;
    int32_t cursor_column;
    int32_t cursor_row;
    bool cursor_visible;
} efi_so_mode_t;

typedef struct efi_so {
    efi_status_t (*reset)(struct efi_so*, bool);
    efi_status_t (*output_string)(struct efi_so*, uint16_t*);
    efi_status_t (*test_string)(struct efi_so*, uint16_t*);

    efi_status_t (*query_mode)(struct efi_so*, size_t, size_t*, size_t*);
    efi_status_t (*set_mode)(struct efi_so*, size_t);
    efi_status_t (*set_attribute)(struct efi_so*, size_t);

    efi_status_t (*clear_screen)(struct efi_so*);
    efi_status_t (*set_cursor_position)(struct efi_so*, size_t, size_t);
    efi_status_t (*enable_cursor)(struct efi_so*, bool);

    efi_so_mode_t *mode;
} efi_so_t;

efi_status_t init_con(void);