#pragma once

#define PAGE_SIZE 0x1000

// FIXME: This stuff should not be in the common.h for all files
// This is only stuff specific to paging
#define P4_ENTRY(addr) (((addr) >> 39) & 0x1FF)
#define P3_ENTRY(addr) (((addr) >> 30) & 0x1FF)
#define P2_ENTRY(addr) (((addr) >> 21) & 0x1FF)
#define P1_ENTRY(addr) (((addr) >> 12) & 0x1FF)
#define KERNEL_PT_ENTRY 0b11
#define USER_PT_ENTRY 0b111
#define SIGN_MASK 0x000ffffffffff000
