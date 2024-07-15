#include <con.h>
#include <loader.h>
#include <acpi.h>
#include <graphics.h>
#include <mem.h>
#include <uefi.h>
#include <common.h>
#include <boot.h>
#include <util/logger.h>

efi_system_table_t *gST;
struct boot_info gBI;
uint8_t CPU = 0;

efi_status_t efi_main(void *image_handle, efi_system_table_t *st) {
    efi_status_t status;
    gST = st;
    uintptr_t kernel_entry_point;
    size_t mmap_key;

    status = gST->bs->set_watchdog_timer(0, 0, 0, NULL);
    if (EFI_ERR(status)) return status;

    status = init_con();
    if (EFI_ERR(status)) return status;

    status = init_graphics();
    if (EFI_ERR(status)) return status;

    status = init_acpi();
    if (EFI_ERR(status)) return status;

    status = init_mem();
    if (EFI_ERR(status)) return status;

    status = load_kernel(&kernel_entry_point);
    if (EFI_ERR(status)) return status;

    status = parse_mmap(&mmap_key);
    if (EFI_ERR(status)) return status;

    status = gST->bs->exit_boot_services(image_handle, mmap_key);
    if (EFI_ERR(status)) return status;

    asm volatile (
        "mov %0, %%cr3;"
        "mov %1, %%rsp;"
        "jmp *%2"
        : : "r" (gBI.pml4), "g" (gBI.stack + KERNEL_STACK_SIZE), "r" (kernel_entry_point), "D" (&gBI)
    );
    // Should never reach here
    return EFI_ERR(1);
}