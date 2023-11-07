#pragma once
#include <kernel/acpi.h>
#include <stdint.h>

typedef struct {
    uint32_t gdt64;
    uint32_t pml4;
    uintptr_t idtr;
    uintptr_t stack_top;
} __attribute__((packed)) ap_info_t;

void start_aps(madt_t *madt);
