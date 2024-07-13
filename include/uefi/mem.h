#pragma once
#include <uefi.h>
#include <boot.h>

efi_status_t init_mem(void);
efi_status_t parse_mmap(size_t*);
efi_status_t map_addr(uintptr_t, uintptr_t, pg_tbl_t);
efi_status_t map_range(uintptr_t, uintptr_t, size_t, pg_tbl_t);