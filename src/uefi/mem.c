#include <uefi.h>

efi_status_t alloc_table(uint64_t **table) {
    efi_status_t status;
    status = gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, table);
    if (EFI_ERR(status)) return status;
    
    status = gST->bs->set_mem(*table, PAGE_SIZE, 0);
    if (EFI_ERR(status)) return status;

    return 0;
}

efi_status_t map_addr(uintptr_t phys, uintptr_t virt, uint64_t *pml4) {
    efi_status_t status;
}