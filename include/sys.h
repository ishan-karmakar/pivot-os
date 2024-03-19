#pragma once

#define PAGE_SIZE 4096
#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
#define HIGHER_HALF_OFFSET 0xFFFF800000000000
#define SIZE_TO_PAGES(size) DIV_CEIL(size, PAGE_SIZE)
#define VADDR(addr) (((uintptr_t) (addr)) | HIGHER_HALF_OFFSET)
#define PADDR(addr) (((uintptr_t) (addr) & ~HIGHER_HALF_OFFSET))
#define ALIGN_ADDR(address) ((address) & -PAGE_SIZE)
#define ALIGN_ADDR_UP(address) ALIGN_ADDR((address) + (PAGE_SIZE - 1))
#define DIV_CEIL(num, dividend) (((num) + ((dividend) - 1)) / (dividend))
#define P4_ENTRY(addr) (((addr) >> 39) & 0x1FF)
#define P3_ENTRY(addr) (((addr) >> 30) & 0x1FF)
#define P2_ENTRY(addr) (((addr) >> 21) & 0x1FF)
#define P1_ENTRY(addr) (((addr) >> 12) & 0x1FF)
#define SIGN_MASK 0x000ffffffffff000
#define PAGE_TABLE_ENTRY 0b11
#define TASK_PAGES 4 // Number of pages that each task gets