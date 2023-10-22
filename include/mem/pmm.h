#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/multiboot.h>
#include <stdbool.h>
#define PAGE_ADDR_MASK 0x000ffffffffff000
#define MEM_FLAGS_USER_LEVEL (1 << 2)

void mmap_parse(mb_mmap_t*);
void init_pmm(uintptr_t addr, uint32_t size, mb_mmap_t*);
void *alloc_frame(void);
void pmm_map_physical_memory(void);
void *map_addr(uint64_t physical, uint64_t address, size_t flags);
void *map_range(uintptr_t start_phys, uintptr_t start_virt, size_t num_pages);