#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/multiboot.h>
#include <stdbool.h>
#define FRAMEBUFFER_START 0xFFFFFFFFA0000000 // Framebuffer gets 16 pages
#define PAGE_ADDR_MASK 0x000ffffffffff000
#define MEM_FLAGS_USER_LEVEL (1 << 2)
#define BITMAP_ENTRY_FULL 0xfffffffffffffff
#define BITMAP_ROW_BITS 64
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