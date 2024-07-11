#pragma once
#include <types.h>

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} efi_allocate_type_t;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiUnacceptedMemoryType,
    EfiMaxMemoryType
} efi_memory_type_t;

efi_status_t init_mem(void);
efi_status_t parse_mmap(void);
efi_status_t get_mmap(size_t*);
efi_status_t alloc_table(uint64_t**);
efi_status_t map_addr(uintptr_t, uintptr_t, uint64_t*);
efi_status_t map_range(uintptr_t, uintptr_t, size_t, uint64_t*);