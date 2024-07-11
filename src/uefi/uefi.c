#include <types.h>
#include <uefi.h>
#include <common.h>
#include <graphics.h>
#include <acpi.h>
#include <loader.h>
#include <util/logger.h>
#include <io/stdio.h>

efi_system_table_t *gST;
boot_info_t gBI;
uint8_t CPU = 0;

efi_status_t efi_main(void *image_handle, efi_system_table_t *st) {
    efi_status_t status;
    gST = st;
    uintptr_t kernel_entry_point;
    uintptr_t stack;
    size_t mmap_key;

    status = gST->bs->set_watchdog_timer(0, 0, 0, NULL);
    if (EFI_ERR(status)) return status;

    status = gST->con_in->reset(gST->con_in, false);
    if (EFI_ERR(status)) return status;

    status = init_con();
    if (EFI_ERR(status)) return status;

    status = init_graphics();
    if (EFI_ERR(status)) return status;

    // status = init_acpi();
    // if (EFI_ERR(status)) return status;

    status = init_mem();
    if (EFI_ERR(status)) return status;

    status = load_kernel(&kernel_entry_point);
    if (EFI_ERR(status)) return status;

    status = gST->bs->alloc_pages(AllocateAnyPages, EfiLoaderData, 1, &stack);
    if (EFI_ERR(status)) return status;

    status = map_range(stack, VADDR(stack), 1, gBI.pml4);
    if (EFI_ERR(status)) return status;

    stack += PAGE_SIZE;

    status = get_mmap(&mmap_key);
    if (EFI_ERR(status)) return status;

    status = parse_mmap();
    if (EFI_ERR(status)) return status;

    status = free_mmap();
    if (EFI_ERR(status)) return status;

    status = get_mmap(&mmap_key);
    if (EFI_ERR(status)) return status;

    // status = gST->bs->exit_boot_services(image_handle, mmap_key);
    // if (EFI_ERR(status)) return status;
    uint64_t *old_cr3;
    uint64_t *new_cr3 = gBI.pml4;
    asm volatile ("mov %%cr3, %0" : "=r" (old_cr3));
    // for (int p4 = 0; p4 < 1; p4++) {
    //     uint64_t *old_p3 = (uint64_t*) (old_cr3[p4] & SIGN_MASK);
    //     uint64_t *new_p3 = (uint64_t*) (new_cr3[p4] & SIGN_MASK);
    //     for (int p3 = 0; p3 < 512; p3++) {
    //         uint64_t *old_p2 = (uint64_t*) (old_p3[p3] & SIGN_MASK);
    //         uint64_t *new_p2 = (uint64_t*) (new_p3[p3] & SIGN_MASK);
    //         for (int p2 = 0; p2 < 512; p2++) {
    //             uint64_t *old_p1 = (uint64_t*) (old_p2[p2] & SIGN_MASK);
    //             uint64_t *new_p1 = (uint64_t*) (new_p2[p2] & SIGN_MASK);
    //             for (int p1 = 0; p1 < 512; p1++) {
    //                 uintptr_t old_addr = old_p1[p1] & SIGN_MASK;
    //                 uintptr_t new_addr = new_p1[p1] & SIGN_MASK;
    //                 if (new_addr != old_addr)
    //                     log(Info, "EFI", "Mismatch - OLD: %x - NEW: %x", old_addr, new_addr);
    //             }
    //         }
    //     }
    // }
    new_cr3[0] = old_cr3[0];

    asm volatile (
        "mov %0, %%cr3\n"
        : : "r" (gBI.pml4)
    );
    uint32_t *test = (uint32_t*) kernel_entry_point;
    for (int i = 0; i < 4; i++)
        log(Verbose, "EFI", "%x", test[i]);
    while(1);
    void (*kernel_entry)(void) = (void (*)(void)) kernel_entry_point;
    kernel_entry();
    while(1);
    return ERR(1);
}