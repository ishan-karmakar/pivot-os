#pragma once
#include <stdint.h>
#include <stddef.h>
#include <boot/con.h>

typedef size_t efi_status_t;

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t rsv;
} efi_table_header_t;

typedef struct {
    efi_table_header_t header;

    uint16_t *firmware_vendor;
    uint32_t firmware_revision;

    void *console_in_handle;
    efi_simple_input_t *con_in;

    void *console_out_handle;
    efi_simple_output_t *con_out;

    void *standard_error_handle;
    efi_simple_output_t *std_err;
} efi_system_table_t;