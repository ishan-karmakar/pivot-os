#include <uefi.h>
#include <common.h>
#include <mem.h>

efi_status_t alloc_table(uint64_t **table) {
    efi_status_t status;
    status = gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, (uintptr_t*) table);
    if (EFI_ERR(status)) return status;
    
    gST->bs->set_mem(*table, PAGE_SIZE, 0);
    if (EFI_ERR(status)) return status;

    return 0;
}

efi_status_t map_addr(uintptr_t phys, uintptr_t virt, uint64_t *p4_tbl) {
    efi_status_t status;

    uint16_t p4_idx = P4_ENTRY(virt),
        p3_idx = P3_ENTRY(virt),
        p2_idx = P2_ENTRY(virt),
        p1_idx = P1_ENTRY(virt);

    uint64_t *table;
    if (!(p4_tbl[p4_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p4_tbl[p4_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p3_tbl = (uint64_t*) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p3_tbl[p3_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p2_tbl = (uint64_t*) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p2_tbl[p2_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    uint64_t *p1_tbl = (uint64_t*) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys | KERNEL_PT_ENTRY;
    return 0;
}

efi_status_t map_range(uintptr_t phys, uintptr_t virt, size_t pages, uint64_t *p4_tbl) {
    efi_status_t status;
    for (size_t i = 0; i < pages; i++) {
        status = map_addr(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, p4_tbl);
        if (EFI_ERR(status)) return status;
    }
    return 0;
}

efi_status_t init_mem(void) {
    efi_status_t status;
    uint64_t *p4_tbl = NULL;
    gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, (uintptr_t*) &p4_tbl);
    gST->bs->set_mem(p4_tbl, PAGE_SIZE, 0);

    status = map_addr((uintptr_t) p4_tbl, (uintptr_t) p4_tbl, p4_tbl);
    if (EFI_ERR(status)) return status;

    return 0;
}

efi_status_t get_mmap(size_t *mmap_key) {
    uint32_t desc_version;
    efi_status_t status;
    status = gST->bs->get_mmap(&gBI.mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (EFI_ERR(status) && status != ERR(5)) return status;

    gBI.mmap_size += 2 * gBI.desc_size;
    status = gST->bs->alloc_pool(EfiLoaderData, gBI.mmap_size, (void**) &gBI.mmap);
    if (EFI_ERR(status)) return status;

    status = gST->bs->get_mmap(&gBI.mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (EFI_ERR(status)) return status;

    return 0;
}

efi_status_t parse_mmap(void) {
    efi_status_t status;
    size_t num_entries = gBI.mmap_size / gBI.desc_size;
    mmap_desc_t *cur_desc = gBI.mmap;
    uintptr_t max_addr = 0;
    for (size_t i = 0; i < num_entries; i++) {
        uintptr_t new_max_addr = cur_desc->physical_start + cur_desc->count * PAGE_SIZE;
        uint32_t t = cur_desc->type;
        if (new_max_addr > max_addr)
            max_addr = new_max_addr;
        if (t == 1 || t == 2 || t == 3 || t == 4 || t == 7 || t == 9 && cur_desc->physical_start != 0)
            map_range(cur_desc->physical_start, cur_desc->physical_start, cur_desc->count, gBI.pml4);
        cur_desc = (mmap_desc_t*) ((char*) cur_desc + gBI.desc_size);
    }
    size_t mem_pages = max_addr / PAGE_SIZE;
    gBI.mem_pages = mem_pages;
    return 0;
}

efi_status_t free_mmap(void) {
    efi_status_t status = gST->bs->free_pool(gBI.mmap);
    if (EFI_ERR(status)) return status;
    return 0;
}