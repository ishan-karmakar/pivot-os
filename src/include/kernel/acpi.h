#pragma once
#include <kernel/multiboot.h>
#pragma pack(1)

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
} rsdp_descriptor_t;

typedef struct {
    rsdp_descriptor_t rsdpv1;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} rsdp_descriptor2_t;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oemtableid[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} sdt_header_t;

typedef struct {
    sdt_header_t header;
    uint32_t lapic_base;
    uint32_t flags;
} madt_t;

typedef struct {
    uint8_t type;
    uint8_t length;
} madt_item_t;

#pragma pack()

void init_acpi(mb_tag_t*);
sdt_header_t *get_table(char*);
void print_madt(madt_t*);