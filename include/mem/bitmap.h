#pragma once
#include <stdint.h>
#include <stddef.h>
#include <boot.h>
#define BITMAP_ROW_BITS 64

void init_bitmap(boot_info_t *boot_info);
void bitmap_set_bit(uintptr_t);
void bitmap_rsv_area(uintptr_t start, size_t size);
void *alloc_frame(void);
