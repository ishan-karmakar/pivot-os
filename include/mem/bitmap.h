#pragma once
#include <stdint.h>
#include <stddef.h>
#define BITMAP_ROW_BITS 64

struct mem_info;

void init_bitmap(struct mem_info *mem_info);
void bitmap_set_bit(uintptr_t);
void bitmap_clear_bit(uintptr_t);
void bitmap_rsv_area(uintptr_t start, size_t size);
int64_t bitmap_request_frame(void);