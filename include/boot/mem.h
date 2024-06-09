#pragma once
#include <kernel.h>
#include <efi.h>
#include <efilib.h>

EFI_STATUS MapAddr(EFI_PHYSICAL_ADDRESS phys_addr, EFI_VIRTUAL_ADDRESS virt_addr, UINT64 *p4_tbl);
EFI_STATUS MapRange(EFI_PHYSICAL_ADDRESS phys_addr, EFI_VIRTUAL_ADDRESS virt_addr, UINTN pages, UINT64 *p4_tbl);
EFI_STATUS ConfigurePaging(kernel_info_t *kinfo);
EFI_STATUS GetMMAP(kernel_info_t *kinfo, UINTN *mmap_key);
EFI_STATUS ParseMMAP(kernel_info_t *kinfo);
EFI_STATUS FreeMMAP(kernel_info_t *kinfo);