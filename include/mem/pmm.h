#pragma once
#include <boot.h>
#include <stddef.h>
#include <sys.h>

extern size_t mem_pages;
extern uint64_t *p4_tbl;

void init_pmm(mem_info_t*);
void *alloc_frame(void);
void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, uint64_t *p4_tbl);
void unmap_addr(uintptr_t virtual, uint64_t *p4_tbl);
void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, uint64_t *p4_tbl);
void map_kernel_entries(uint64_t*);