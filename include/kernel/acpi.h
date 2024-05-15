#pragma once
#include <stdint.h>

#define MADT_LAPIC 0
#define MADT_IOAPIC 1
#define MADT_INT_SO_OVRD 2
#define MADT_NMI 3
#define MADT_LAPIC_NMI 4
#define MADT_LAPIC_ADDR_OVRD 5

struct boot_info;

#pragma pack(push, default)
#pragma pack(1)

typedef struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t rsv[3];
} rsdp_descriptor_t;

typedef struct sdt_header {
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

typedef struct xsdt {
    sdt_header_t header;
    uintptr_t tables[0];
} xsdt_t;

typedef struct rsdt {
    sdt_header_t header;
    uint32_t tables[0];
} rsdt_t;

typedef struct madt_item {
    uint8_t type;
    uint8_t length;
} madt_item_t;

typedef struct madt {
    sdt_header_t header;
    uint32_t lapic_base;
    uint32_t flags;
} madt_t;

#pragma pack(pop)

void init_acpi(struct boot_info*);
sdt_header_t *get_table(char*);
madt_item_t *get_madt_item(madt_t *table, uint8_t search_item, uint8_t count);