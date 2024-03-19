#pragma once
#include <stdint.h>
#include <stddef.h>
#define BITMAP_ROW_BITS 64
#define BITMAP_ROW_FULL UINT64_MAX

void init_bitmap(void);
void bitmap_set_bit(uintptr_t);
void bitmap_clear_bit(uintptr_t);
void bitmap_rsv_area(uintptr_t start, size_t num_pages);
void bitmap_clear_area(uintptr_t start, size_t num_pages);
int64_t bitmap_request_frame(void);