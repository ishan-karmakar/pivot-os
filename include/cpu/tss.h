#pragma once
#include <stdint.h>
#include <mem/heap.h>

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

void init_tss(heap_t);
void set_rsp0(uintptr_t);