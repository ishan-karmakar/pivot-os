#pragma once
#include <boot.h>
#include <stddef.h>
#include <sys.h>

extern size_t mem_pages;

void init_pmm(boot_info_t*);
void *alloc_frame(void);
void *alloc_range(size_t);
void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags);
void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages);
void bitmap_rsv_area(uintptr_t start, size_t num_pages);
void map_phys_mem(void);