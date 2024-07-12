#pragma once

#define PAGE_SIZE 0x1000
#define KERNEL_STACK_SIZE PAGE_SIZE
#define KERNEL_STACK_PAGES DIV_CEIL(KERNEL_STACK_SIZE, PAGE_SIZE)
#define DIV_CEIL(num, dividend) (((num) + ((dividend) - 1)) / (dividend))
#define ALIGN_ADDR(address) ((address) & -PAGE_SIZE)
#define HIGHER_HALF_OFFSET 0xFFFF800000000000
#define VADDR(addr) (((uintptr_t) (addr)) | HIGHER_HALF_OFFSET)
#define PADDR(addr) (((uintptr_t) (addr) & ~HIGHER_HALF_OFFSET))

// FIXME: This stuff should not be in the common.h for all files
// This is only stuff specific to paging!
#define P4_ENTRY(addr) (((addr) >> 39) & 0x1FF)
#define P3_ENTRY(addr) (((addr) >> 30) & 0x1FF)
#define P2_ENTRY(addr) (((addr) >> 21) & 0x1FF)
#define P1_ENTRY(addr) (((addr) >> 12) & 0x1FF)
#define KERNEL_PT_ENTRY 0b11
#define USER_PT_ENTRY 0b111
#define SIGN_MASK 0x000ffffffffff00
