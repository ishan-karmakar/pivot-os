#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/multiboot.h>
#include <stdbool.h>
#define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
#define HIGHER_HALF_OFFSET 0xFFFF800000000000
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
#define MAKE_HIGHER_HALF(addr) ((addr) < HIGHER_HALF_OFFSET ? (addr) + HIGHER_HALF_OFFSET : (addr))
#define MAKE_PHYS_ADDR(addr) ((addr) >= HIGHER_HALF_OFFSET ? (addr) - HIGHER_HALF_OFFSET : (addr))
#define BITMAP_ENTRY_FULL 0xfffffffffffffff
#define BITMAP_ROW_BITS 64
#define ALIGN_ADDR(address) ((address) & ~(PAGE_SIZE - 1))
#define ALIGN_ADDR_UP(address) (((address) + PAGE_SIZE) / (PAGE_SIZE + 1) * PAGE_SIZE)
#define PAGES_PER_TABLE 512
#define VMM_RESERVED_SPACE 0x14480000000
#define VMM_FLAGS_PRESENT 1
#define VMM_FLAGS_WRITE_ENABLE (1 << 1)
#define VMM_FLAGS_USER_LEVEL (1 << 2)
#define VMM_FLAGS_ADDRESS_ONLY (1 << 7)
#define KHEAP_ALLOC_ALIGNMENT 0x10
#define KHEAP_ALIGN(size) ((size) / KHEAP_ALLOC_ALIGNMENT + 1) * KHEAP_ALLOC_ALIGNMENT
#define KHEAP_MIN_ALLOC_SIZE 0x20
#define MERGE_LEFT 0b10
#define MERGE_RIGHT 0b1

typedef struct kheap_node_t {
    size_t size;
    bool free;
    struct kheap_node_t *next, *prev;
} kheap_node_t;

void mmap_parse(mb_mmap_t*);
void init_pmm(uintptr_t addr, uint32_t size, uint64_t mem_size);
void init_kheap(void);
void *kmalloc(size_t);
void *alloc_frame(void);
void pmm_map_physical_memory(void);
void *map_addr(uint64_t physical, uint64_t address, size_t flags);
void bitmap_set_bit_addr(uint64_t address);
void bitmap_set_bit(uint64_t location);