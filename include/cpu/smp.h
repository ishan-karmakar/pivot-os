#pragma once
#include <acpi/acpi.h>
#include <stddef.h>

typedef struct {
    uintptr_t pml4;
    uint32_t stack_top; // +4
    uintptr_t gdtr; // +8
    uintptr_t idtr; // +16
    uint8_t ready;
} __attribute__((packed)) ap_info_t;

void start_aps(void);
void set_ap_ready(void);