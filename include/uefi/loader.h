#pragma once
#include <uefi.h>

/**
 * @brief The 64-bit ELF header.
 */
typedef struct s_elf64_hdr {
	unsigned char	e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} elf64_ehdr_t;

/**
 * @brief The 64-bit ELF program header.
 */
typedef struct s_elf64_phdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
} elf64_phdr_t;

typedef struct efi_file_handle {
    uint64_t revision;
    efi_status_t (*open)(struct efi_file_handle*, struct efi_file_handle**, uint16_t*, uint64_t, uint64_t);
    uintptr_t close;
    uintptr_t delete;
    efi_status_t (*read)(struct efi_file_handle*, size_t *buffer_size, void *buffer);
    uintptr_t write;
    uintptr_t get_pos;
    efi_status_t (*set_pos)(struct efi_file_handle*, uint64_t);
    uintptr_t get_info;
    uintptr_t set_info;
    uintptr_t flush;
    uintptr_t openex;
    uintptr_t readex;
    uintptr_t writeex;
    uintptr_t flushex;
} efi_file_handle_t;

typedef struct efi_sfsp {
    uint64_t revision;
    efi_status_t (*open_volume)(struct efi_sfsp*, efi_file_handle_t**);
} efi_sfsp_t;

efi_status_t load_kernel(uintptr_t *entry);