#pragma once
#include <efi.h>
#include <efilib.h>
#include <kernel.h>

#pragma pack(push, default)
#pragma pack(1)

/**
 * @brief The 64-bit ELF header.
 */
typedef struct s_elf64_hdr {
	unsigned char	e_ident[16];
	UINT16 e_type;
	UINT16 e_machine;
	UINT32 e_version;
	UINT64 e_entry;
	UINT64 e_phoff;
	UINT64 e_shoff;
	UINT32 e_flags;
	UINT16 e_ehsize;
	UINT16 e_phentsize;
	UINT16 e_phnum;
	UINT16 e_shentsize;
	UINT16 e_shnum;
	UINT16 e_shstrndx;
} elf64_ehdr_t;

/**
 * @brief The 64-bit ELF program header.
 */
typedef struct s_elf64_phdr {
	UINT32 p_type;
	UINT32 p_flags;
	UINT64 p_offset;
	UINT64 p_vaddr;
	UINT64 p_paddr;
	UINT64 p_filesz;
	UINT64 p_memsz;
	UINT64 p_align;
} elf64_phdr_t;

#pragma pack(pop)

EFI_STATUS LoadKernel(kernel_info_t *kinfo, EFI_PHYSICAL_ADDRESS *kernel_entry_point);