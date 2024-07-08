#include <uefi.h>
#include <common.h>
#include <util/logger.h>

efi_status_t alloc_table(uint64_t **table) {
    efi_status_t status;
    status = gST->boot_services->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, (uintptr_t*) table);
    if (EFI_ERR(status)) return status;

    gST->boot_services->set_mem(*table, PAGE_SIZE, 0);
    return 0;
}

efi_status_t map_addr(uintptr_t phys, uintptr_t virt, size_t flags) {
    efi_status_t status;
    size_t p4_idx = P4_ENTRY(virt);
    size_t p3_idx = P3_ENTRY(virt);
    size_t p2_idx = P2_ENTRY(virt);
    size_t p1_idx = P1_ENTRY(virt);

    uint64_t *p4_tbl = gBI.pml4;
    if (!(p4_tbl[p4_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p4_tbl[p4_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p3_tbl = (uint64_t*) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p3_tbl[p3_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p2_tbl = (uint64_t*) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        uint64_t *table;
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p2_tbl[p2_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;

        status = map_addr((uintptr_t) table, (uintptr_t) table, KERNEL_PT_ENTRY);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p1_tbl = (uint64_t*) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys | flags;

    return 0;
}

efi_status_t map_range(uintptr_t phys, uintptr_t virt, size_t flags, size_t pages) {
    efi_status_t status;
    for (size_t i = 0; i < pages; i++) {
        status = map_addr(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags);
        if (EFI_ERR(status)) return status;
    }
    return 0;
}

efi_status_t init_mem(void) {
    efi_status_t status;
    uint64_t *pml4 = NULL;

    status = alloc_table(&pml4);
    if (EFI_ERR(status)) return status;

    gBI.pml4 = pml4;
    status = map_addr((uintptr_t) pml4, (uintptr_t) pml4, KERNEL_PT_ENTRY);
    if (EFI_ERR(status)) return status;
    log(Info, "MEM", "Identity mapped PML4");

    status = gST->boot_services->alloc_pages(AllocateAnyPages, EfiLoaderData, DIV_CEIL(KERNEL_STACK_SIZE, PAGE_SIZE), &gBI.stack);
    if (EFI_ERR(status)) return status;
    status = map_addr(gBI.stack, VADDR(gBI.stack), KERNEL_PT_ENTRY);
    gBI.stack = VADDR(gBI.stack);
    log(Info, "MEM", "Allocated kernel stack");
    return 0;
}

efi_status_t get_mmap(size_t *mmap_key) {
    efi_status_t status;
    uint32_t desc_version;
    gBI.mmap_size = 0;

    status = gST->boot_services->get_mmap(&gBI.mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (status != (5 | ERR_MASK) && EFI_ERR(status)) return status;

    gBI.mmap_size += 2 * gBI.desc_size;
    status = gST->boot_services->alloc_pool(EfiLoaderData, gBI.mmap_size, (void**) &gBI.mmap);
    if (EFI_ERR(status)) return status;

    status = gST->boot_services->get_mmap(&gBI.mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (EFI_ERR(status)) return status;
    return 0;
}

efi_status_t parse_mmap(uint64_t *mmap_key) {
    efi_status_t status;
    status = get_mmap(mmap_key);
    if (EFI_ERR(status)) return status;

    size_t num_entries = gBI.mmap_size / gBI.desc_size;
    mmap_desc_t *cur_desc = gBI.mmap;
    uintptr_t max_addr = 0;
    for (size_t i = 0; i < num_entries; i++) {
        uintptr_t new_addr = cur_desc->physical_start + cur_desc->count * PAGE_SIZE;
        uint32_t t = cur_desc->type;
        if (new_addr > max_addr)
            max_addr = new_addr;
        if (t == 1 || t == 2 || t == 3 || t == 4 || t == 7 || t == 9) {
            if (cur_desc->physical_start == 0)
                map_range(PAGE_SIZE, PAGE_SIZE, KERNEL_PT_ENTRY, cur_desc->count - 1);
            else
                map_range(cur_desc->physical_start, cur_desc->physical_start, KERNEL_PT_ENTRY, cur_desc->count);
        }

        cur_desc = (mmap_desc_t*) ((uintptr_t) cur_desc + gBI.desc_size);
    }

    cur_desc = gBI.mmap;

    gBI.mem_pages = max_addr / PAGE_SIZE;
    log(Info, "MMAP", "Mapped necessary regions");

    status = gST->boot_services->free_pool(gBI.mmap);
    if (status < 0) return status;
    status = get_mmap(mmap_key);
    if (status < 0) return status;
    return 0;
}