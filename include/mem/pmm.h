#pragma once
#include <boot.h>
#include <stddef.h>
#include <sys.h>

typedef uint64_t* page_table_t;

extern mem_info_t *mem_info;

void init_pmm(mem_info_t*);
void map_higher_half(void);
void *alloc_frame(void);
void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, uint64_t *p4_tbl);
void unmap_addr(uintptr_t virtual, uint64_t *p4_tbl);
void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, uint64_t *p4_tbl);
void map_kernel_entries(uint64_t*);
bool addr_in_phys_mem(uintptr_t);
void cleanup_uefi(void);
void clean_table(uint64_t*);
void invlpg(uintptr_t);
uintptr_t get_phys_addr(uintptr_t, page_table_t);