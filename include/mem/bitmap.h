#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/multiboot.h>

#define BITMAP_ENTRY_FULL 0xfffffffffffffff
#define BITMAP_ROW_BITS 64

void initialize_bitmap(uint64_t rsv_end, mb_mmap_entry_t*, uint32_t);
void bitmap_set_bit_addr(uint64_t address);
void bitmap_rsv_area(uint64_t start, size_t size);