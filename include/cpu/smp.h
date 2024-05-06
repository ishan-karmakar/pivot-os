#pragma once
#include <kernel/acpi.h>

typedef struct {
    uint32_t pml4;
    uint32_t stack_top; // +4
    uint64_t gdtr; // +8
    uint64_t idtr; // +16
} __attribute__((packed)) ap_info_t;

void start_aps(void);