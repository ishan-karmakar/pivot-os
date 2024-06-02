#pragma once
#include <boot.h>
#include <stddef.h>
#include <sys.h>
#define BITMAP_ROW_FULL 0xFFFFFFFFFFFFFFFF
#define BITMAP_ROW_BITS 64

typedef uint64_t* page_table_t;

extern mem_info_t *mem_info;

void init_pmm(mem_info_t*);
void *alloc_frame(void);
void map_addr(uintptr_t physical, uintptr_t virtual, size_t flags, page_table_t p4_tbl);
void unmap_addr(uintptr_t virtual, page_table_t p4_tbl);
void map_range(uintptr_t physical, uintptr_t virtual, size_t num_pages, size_t flags, page_table_t p4_tbl);
void map_kernel_entries(uint64_t*);
void cleanup_uefi(void);
void clean_table(uint64_t*);
void invlpg(uintptr_t);
uintptr_t get_phys_addr(uintptr_t, page_table_t);
void free_page_table(page_table_t, uint8_t);

void pmm_set_bit(uintptr_t);
void pmm_clear_bit(uintptr_t);
bool pmm_check_bit(uintptr_t);
void pmm_set_area(uintptr_t start, size_t num_pages);
void pmm_clear_area(uintptr_t start, size_t num_pages);