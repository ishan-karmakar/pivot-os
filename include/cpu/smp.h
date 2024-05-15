#pragma once
#include <kernel/acpi.h>

typedef struct {
    uintptr_t pml4;
    uint32_t stack_top; // +4
    uintptr_t gdtr; // +8
    uintptr_t idtr; // +16
    uint8_t ready;
    /*
    0 - Reserved
    1 - Refresh entire CR3
    2 - INVLPG single addr
    */
    uint8_t action;
    uintptr_t invl_page;
} __attribute__((packed)) ap_info_t;

void start_aps(void);
void set_ap_ready(void);