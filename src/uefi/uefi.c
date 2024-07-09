#include <types.h>
#include <uefi.h>
#include <common.h>
#include <graphics.h>
#include <acpi.h>
#include <loader.h>

efi_system_table_t *gST;
boot_info_t gBI;

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

    status = gST->bs->exit_boot_services(image_handle, mmap_key);
    if (EFI_ERR(status)) return status;
    while(1);
    return ERR(1);
}