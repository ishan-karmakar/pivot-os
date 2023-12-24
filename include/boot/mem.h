#pragma once
#include <boot.h>
#include <efi.h>
#include <efilib.h>

EFI_STATUS MapAddr(EFI_VIRTUAL_ADDRESS virt_addr, EFI_PHYSICAL_ADDRESS phys_addr, UINT64 *p4_tbl);
EFI_STATUS ConfigurePaging(boot_info_t *boot_info);
void LoadCr3(boot_info_t *boot_info);
EFI_STATUS GetMMAP(boot_info_t *boot_info, UINTN *mmap_key);