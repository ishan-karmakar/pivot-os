#include <boot/mem.h>
#include <sys.h>
#define MAX_MAPPED_ADDR 0x40000000

EFI_STATUS AllocTable(UINT64 **table) {
    EFI_STATUS status;
    *table = (UINT64*) MAX_MAPPED_ADDR;
    status = uefi_call_wrapper(gBS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData, 1, table);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating a page under 1 MiB\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->SetMem, 3, *table, EFI_PAGE_SIZE, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error zeroing page table\n");
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS MapAddr(EFI_VIRTUAL_ADDRESS virt_addr, EFI_PHYSICAL_ADDRESS phys_addr, UINT64 *p4_tbl) {
    EFI_STATUS status;
    UINTN p4_idx = P4_ENTRY(virt_addr);
    UINTN p3_idx = P3_ENTRY(virt_addr);
    UINTN p2_idx = P2_ENTRY(virt_addr);
    UINTN p1_idx = P1_ENTRY(virt_addr);
    if (!(p4_tbl[p4_idx] & 1)) {
        UINT64 *table;
        status = AllocTable(&table);
        if (EFI_ERROR(status))
            return status;
        p4_tbl[p4_idx] = (EFI_PHYSICAL_ADDRESS) table | 0b11;
    }

    UINT64 *p3_tbl = (UINT64*)(p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        UINT64 *table;
        status = AllocTable(&table);
        if (EFI_ERROR(status))
            return status;
        p3_tbl[p3_idx] = (EFI_PHYSICAL_ADDRESS) table | 0b11;
    }

    UINT64 *p2_tbl = (UINT64*)(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        UINT64 *table;
        status = AllocTable(&table);
        if (EFI_ERROR(status))
            return status;
        p2_tbl[p2_idx] = (EFI_PHYSICAL_ADDRESS) table | 0b11;
    }

    UINT64 *p1_tbl = (UINT64*)(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys_addr | 0b11;
    return EFI_SUCCESS;
}

EFI_STATUS ConfigurePaging(boot_info_t *boot_info) {
    EFI_STATUS status;
    UINT64 *p4_tbl = (UINT64*) MAX_MAPPED_ADDR;
    uefi_call_wrapper(gBS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData, 1, &p4_tbl);
    uefi_call_wrapper(gBS->SetMem, 3, p4_tbl, EFI_PAGE_SIZE, 0);
    boot_info->pml4 = p4_tbl;
    Print(L"PML4 Address: 0x%x\n", (EFI_PHYSICAL_ADDRESS) p4_tbl);

    for (UINTN i = 0; i < 262144; i++) { // 64 mb
        EFI_PHYSICAL_ADDRESS addr = i * EFI_PAGE_SIZE;
        status = MapAddr(addr, addr, p4_tbl);
        if (EFI_ERROR(status))
            return status;
        status = MapAddr(VADDR(addr), addr, p4_tbl);
        if (EFI_ERROR(status))
            return status;
    }
    Print(L"Mapped first GiB of memory\n");

    return EFI_SUCCESS;
}

void LoadCr3(boot_info_t *boot_info) {
    asm volatile (
        "mov %0, %%rax\n\t"
        "mov %%rax, %%cr3"
        : : "r" ((EFI_PHYSICAL_ADDRESS) boot_info->pml4) : "rax"
    );
}

EFI_STATUS GetMMAP(boot_info_t *boot_info, UINTN *mmap_key) {
    EFI_MEMORY_DESCRIPTOR *mmap = NULL;
    UINTN mmap_size = 0;
    UINTN descriptor_size = 0;
    UINT32 descriptor_version = 0;
    EFI_STATUS status;
    status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &mmap_size, mmap, mmap_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL) {
        Print(L"Error getting memory map\n");
        return status;
    }

    mmap_size += 2 * descriptor_size;
    status = uefi_call_wrapper(gBS->AllocatePool, 3, EfiLoaderData, mmap_size, &mmap);
    if (EFI_ERROR(status)) {
        Print(L"Error allocating memory for MMAP\n");
        return status;
    }

    status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &mmap_size, mmap, mmap_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) {
        Print(L"Error getting MMAP\n");
        return status;
    }

    boot_info->mmap = (mmap_descriptor_t*) (mmap);
    boot_info->mmap_size = mmap_size;
    boot_info->mmap_descriptor_size = descriptor_size;

    return EFI_SUCCESS;
}