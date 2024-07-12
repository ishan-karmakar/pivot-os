#pragma once
#include <types.h>

efi_status_t init_mem(void);
efi_status_t parse_mmap(size_t*);
efi_status_t alloc_table(uint64_t**);
efi_status_t map_addr(uintptr_t, uintptr_t, uint64_t*);
efi_status_t map_range(uintptr_t, uintptr_t, size_t, uint64_t*);