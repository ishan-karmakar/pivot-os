#pragma once
#include <acpi/acpi.h>

#pragma pack(push, 1)

typedef struct madt_item {
    uint8_t type;
    uint8_t length;
} madt_item_t;

typedef struct madt {
    sdt_header_t header;
    uint32_t lapic_base;
    uint32_t flags;
} madt_t;

typedef struct ioapic_so {
    madt_item_t header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi_base;
    uint16_t flags;
} ioapic_so_t;

typedef struct ioapic {
    madt_item_t header;
    uint8_t id;
    uint8_t rsv;
    uint32_t addr;
    uint32_t gsi_base;
} ioapic_t;

typedef struct lapic {
    madt_item_t header;
    uint8_t acpi_id;
    uint8_t id;
    uint32_t flags;
} lapic_t;

#pragma pack(pop)

void parse_madt(sdt_header_t*);
