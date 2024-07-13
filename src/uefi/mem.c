#include <uefi.h>
#include <boot.h>
#include <common.h>
#include <mem.h>
#include <util/logger.h>

efi_status_t alloc_table(pg_tbl_t *table) {
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

    pg_tbl_t table;
    if (!(p4_tbl[p4_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p4_tbl[p4_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    pg_tbl_t p3_tbl = (pg_tbl_t) (p4_tbl[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p3_tbl[p3_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    pg_tbl_t p2_tbl = (pg_tbl_t) (p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        status = alloc_table(&table);
        if (EFI_ERR(status)) return status;
        p2_tbl[p2_idx] = (uintptr_t) table | KERNEL_PT_ENTRY;
        status = map_addr((uintptr_t) table, (uintptr_t) table, p4_tbl);
        if (EFI_ERR(status)) return status;
    }

    pg_tbl_t p1_tbl = (pg_tbl_t) (p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys | KERNEL_PT_ENTRY;
    return 0;
}

efi_status_t map_range(uintptr_t phys, uintptr_t virt, size_t pages, pg_tbl_t p4_tbl) {
    efi_status_t status;
    for (size_t i = 0; i < pages; i++) {
        status = map_addr(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, p4_tbl);
        if (EFI_ERR(status)) return status;
    }
    return 0;
}

efi_status_t init_mem(void) {
    efi_status_t status;
    pg_tbl_t p4_tbl = NULL;
    gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, (uintptr_t*) &p4_tbl);
    pg_tbl_t uefi_tbl = NULL;
    asm volatile ("mov %%cr3, %0" : "=r" (uefi_tbl));

    for (int i = 0; i < 512; i++)
        p4_tbl[i] = uefi_tbl[i];
    gBI.pml4 = p4_tbl;

    log(Info, "MEM", "Initialized page tables");

    status = gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, KERNEL_STACK_PAGES, &gBI.stack);
    if (EFI_ERR(status)) return status;
    gBI.stack = VADDR(gBI.stack);
    status = map_range(PADDR(gBI.stack), gBI.stack, KERNEL_STACK_PAGES, gBI.pml4);
    if (EFI_ERR(status)) return status;
    log(Info, "MEM", "Allocated kernel stack");
    return 0;
}

efi_status_t parse_mmap(size_t *mmap_key) {
    efi_status_t status;
    uint32_t desc_version;
    size_t mmap_size = 0;
    status = gST->bs->get_mmap(&mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (EFI_ERR(status) && status != ERR(5)) return status;

    mmap_size += 2 * gBI.desc_size;
    status = gST->bs->alloc_pool(EfiLoaderData, mmap_size, (void**) &gBI.mmap);
    if (EFI_ERR(status)) return status;

    status = gST->bs->get_mmap(&mmap_size, gBI.mmap, mmap_key, &gBI.desc_size, &desc_version);
    if (EFI_ERR(status)) return status;

    size_t num_entries = mmap_size / gBI.desc_size;
    struct mmap_desc *cur_desc = gBI.mmap;
    uintptr_t max_addr = 0;
    for (size_t i = 0; i < num_entries; i++) {
        uintptr_t new_max_addr = cur_desc->phys + cur_desc->count * PAGE_SIZE;
        if (new_max_addr > max_addr)
            max_addr = new_max_addr;
        cur_desc = (struct mmap_desc*) ((char*) cur_desc + gBI.desc_size);
    }
    gBI.mem_pages = max_addr / PAGE_SIZE;
    gBI.mmap_entries = mmap_size / gBI.desc_size;
    return 0;
}
