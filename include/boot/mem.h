#pragma once
#include <boot.h>
#include <efi.h>
#include <efilib.h>

EFI_STATUS MapAddr(EFI_PHYSICAL_ADDRESS phys_addr, EFI_VIRTUAL_ADDRESS virt_addr, UINT64 *p4_tbl);
EFI_STATUS ConfigurePaging(mem_info_t *mem_info);
void LoadCr3(mem_info_t *mem_info);
EFI_STATUS GetMMAP(mem_info_t *mem_info, UINTN *mmap_key);
EFI_STATUS ParseMMAP(mem_info_t *mem_info);
EFI_STATUS FreeMMAP(mem_info_t *mem_info);