#pragma once
#include <stdint.h>
#include <stddef.h>
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
#define MEM_FLAGS_USER_LEVEL (1 << 2)
#define PAGE_SIZE 0x200000
#define END_MEMORY ((511UL << 39) | (510UL << 30) | (511 << 21))
#define IS_HIGHER_HALF(addr) ((addr) & (11UL << 62))
#define MAKE_HIGHER_HALF(addr) ((addr) < KERNEL_VIRTUAL_ADDR ? (addr) + KERNEL_VIRTUAL_ADDR : (addr))
#define BITMAP_ENTRY_FULL 0xfffffffffffffff
#define BITMAP_ROW_BITS 64
#define ALIGN_ADDR(address) (address & ~(PAGE_SIZE - 1))
#define PAGES_PER_TABLE 512

typedef enum {
    ADDRESS_TYPE_PHYSICAL,
    ADDRESS_TYPE_VIRTUAL
} address_type_t;

void mmap_parse(mb_mmap_t*);
void init_mem(uintptr_t addr, uint32_t size, uint64_t mem_size);
void *map_addr(uint64_t physical, uint64_t address, size_t flags);
void bitmap_set_bit(uint64_t location);