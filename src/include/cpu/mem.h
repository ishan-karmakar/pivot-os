#pragma once
#include <stdint.h>
#include <kernel/multiboot.h>
#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
#define FRAMEBUFFER_START 0xFFFFFFFFA0000000 // Framebuffer gets 16 pages
#define PML4_ENTRY(address)(((address)>>39) & 0x1ff)
#define PDPR_ENTRY(address)(((address)>>30) & 0x1ff)
#define PD_ENTRY(address)(((address)>>21) & 0x1ff)
#define PAGE_ADDR_MASK 0x000ffffffffff000
#define PRESENT_BIT 1
#define WRITE_BIT 0b10
#define HUGEPAGE_BIT 0b10000000
#define PAGE_TABLE_ENTRY HUGEPAGE_BIT | WRITE_BIT | PRESENT_BIT
#define PAGE_SIZE 0x200000

void init_mem(uintptr_t addr, uint32_t size, size_t mem_size);