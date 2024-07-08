#include <uefi.h>
#include <common.h>
#include <util/logger.h>

efi_status_t alloc_table(uint64_t **table) {
    efi_status_t status;
    status = gST->boot_services->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, table);
    if (status < 0) return status;

    gST->boot_services->set_mem(*table, PAGE_SIZE, 0);
    return 0;
}

efi_status_t map_addr(uintptr_t phys, uintptr_t virt, size_t flags, uint64_t *p4_tbl) {
    efi_status_t status;
    size_t p4_idx = P4_ENTRY(virt);
    size_t p3_idx = P3_ENTRY(virt);
    size_t p2_idx = P2_ENTRY(virt);
    size_t p1_idx = P1_ENTRY(virt);

    if (!(p4_tbl[p4_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (status < 0) return status;
        p4_tbl[p4_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
        if (status < 0) return status;
    }

    uint64_t *p3_tbl = (uint64_t*) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (status < 0) return status;
        p3_tbl[p3_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
        if (status < 0) return status;
    }

    uint64_t *p2_tbl = (uint64_t*) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (status < 0) return status;
        p2_tbl[p2_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY, p4_tbl);
        if (status < 0) return status;
    }

    uint64_t *p1_tbl = (uint64_t*) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys | flags;

    return 0;
}

efi_status_t map_range(uintptr_t phys, uintptr_t virt, size_t flags, uint64_t *p4_tbl, size_t pages) {
    efi_status_t status;
    for (size_t i = 0; i < pages; i++) {
        status = map_addr(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags, p4_tbl);
        if (status < 0) return status;
    }
    return 0;
}

efi_status_t init_mem(efi_boot_services_t *bs) {
    efi_status_t status;
    uint64_t *pml4 = NULL;

    status = alloc_table(&pml4);
    if (status < 0) return status;

    status = map_addr((uintptr_t) pml4, (uintptr_t) pml4, KERNEL_PT_ENTRY, pml4);
    if (status < 0) return status;

    log(Info, "MEM", "Identity mapped PML4");
}