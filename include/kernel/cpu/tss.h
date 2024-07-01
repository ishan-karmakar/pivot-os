#pragma once
#include <stdint.h>
#include <mem/heap.h>
#include <cpu/gdt.h>
#include <util/logger.h>

typedef struct tss {
    uint32_t rsv0;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint64_t rsv1;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t rsv2;
    uint16_t rsv3;

    uint16_t iopb;
} __attribute__((packed)) tss_t;

void init_tss(void);

__attribute__((always_inline))
inline void set_rsp0(void) {
    uintptr_t rsp;
    asm volatile ("mov %%rsp, %0" : "=r" (rsp));
    uint16_t tr;
    asm volatile ("str %0" : "=rm" (tr));
    gdt_desc_t *entry0 = &gdt[tr / 8];
    gdt_desc_t *entry1 = &gdt[tr / 8 + 1];
    tss_t *tss = (tss_t*) (entry0->fields.base0 |
                          (entry0->fields.base1 << 16) |
                          (entry0->fields.base2 << 24) |
                          ((entry1->raw & 0xFFFFFFFF) << 32));
    tss->rsp0 = rsp;
}