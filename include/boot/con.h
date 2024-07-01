#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef size_t efi_status_t;

typedef struct {
    uint16_t scan_code;
    uint16_t unicode_char;
    void *wait_for_key;
} efi_input_key_t;

typedef struct efi_simple_input {
    efi_status_t (*reset)(struct efi_simple_input*, bool);
    efi_status_t (*read_key_stroke)(struct efi_simple_input*, efi_input_key_t*);
} efi_simple_input_t;

typedef struct {
    int32_t max_mode;
    int32_t mode;
    int32_t attribute;
    int32_t cursor_column;
    int32_t cursor_row;
    bool cursor_visible;
} efi_simple_output_mode_t;

typedef struct efi_simple_output {
    efi_status_t (*reset)(struct efi_simple_output*, bool);
    efi_status_t (*output_string)(struct efi_simple_output*, uint16_t*);
    efi_status_t (*test_string)(struct efi_simple_output*, uint16_t*);

    efi_status_t (*query_mode)(struct efi_simple_output*, size_t, size_t*, size_t*);
    efi_status_t (*set_mode)(struct efi_simple_output*, size_t);
    efi_status_t (*set_attribute)(struct efi_simple_output*, size_t);

    efi_status_t (*clear_screen)(struct efi_simple_output*);
    efi_status_t (*set_cursor_position)(struct efi_simple_output*, size_t, size_t);
    efi_status_t (*enable_cursor)(struct efi_simple_output*, bool);

    efi_simple_output_mode_t *mode;
} efi_simple_output_t;