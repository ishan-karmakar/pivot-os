#pragma once
#include <stdint.h>
#include <stdbool.h>
#define IOAPIC_ID_OFFSET 0
#define IOAPIC_VER_OFFSET 1
#define IOAPIC_ARB_OFFSET 2
#define IOAPIC_REDTBL_START_OFFSET 0x10

#define IOAPIC_MAX_SOURCE_OVERRIDE 9

#pragma pack(push, default)
#pragma pack(1)

typedef union {
    struct {
        uint64_t vector:8;
        uint64_t delivery_mode:3;
        uint64_t destination_mode:1;
        uint64_t delivery_status:1;
        uint64_t pin_polarity:1;
        uint64_t remote_irr:1;
        uint64_t trigger_mode:1;
        uint64_t mask:1;
        uint64_t rsv: 39;
        uint64_t destination:8;
    };
    uint64_t raw;
} ioapic_redtbl_entry_t;

typedef struct ioapic_source_override {
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi_base;
    uint16_t flags;
} ioapic_source_override_t;

typedef struct ioapic {
    uint8_t id;
    uint8_t rsv;
    uint32_t addr;
    uint32_t gsi_base;
} ioapic_t;

#pragma pack(pop)

void init_ioapic(void);
void set_irq(uint8_t irq, uint8_t idt_entry, uint8_t destination_field, uint32_t flags, bool masked);
void set_irq_mask(uint8_t, bool);