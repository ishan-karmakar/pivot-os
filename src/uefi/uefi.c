#include <uefi.h>
#include <con.h>
#include <graphics.h>
#include <acpi.h>
#include <util/logger.h>
#include <loader.h>

uint8_t CPU = 0;
efi_system_table_t *gST;
boot_info_t gBI;

efi_status_t verify_table(efi_table_header_t *header) {
    uint32_t crc32, old_crc32 = header->crc32;
    header->crc32 = 0;
    efi_status_t status = gST->boot_services->calculate_crc32(header, header->header_size, &crc32);
    if (EFI_ERR(status)) return status;
    if (crc32 == old_crc32)
        return 0;
    else
        return ERR(27);
}

efi_status_t efi_main(void* image_handle, efi_system_table_t *st) {
    gST = st;
    efi_status_t status;
    status = init_con();
    if (EFI_ERR(status)) return status;
    
    status = verify_table(&st->header);
    if (EFI_ERR(status)) return status;
    log(Info, "EFI", "System table verified");

    status = verify_table(&st->boot_services->header);
    if (EFI_ERR(status)) return status;
    log(Info, "EFI", "Boot services table verified");

    status = init_graphics();
    if (EFI_ERR(status)) return status;

    status = init_acpi();
    if (EFI_ERR(status)) return status;

    status = init_mem();
    if (EFI_ERR(status)) return status;

    uintptr_t entry = 0;
    status = load_kernel(&entry);
    if (EFI_ERR(status)) return status;
    
    uint64_t mmap_key;
    status = parse_mmap(&mmap_key);
    if (EFI_ERR(status)) return status;

    status = gST->boot_services->exit_boot_services(image_handle, mmap_key);
    if (EFI_ERR(status)) return status;

    asm volatile (
        "mov %0, %%cr3\n"
        "mov %1, %%rsp\n"
        : : "r" (gBI.pml4), "g" (gBI.stack)
    );

    // volatile uint8_t test = *(volatile uint8_t*) entry;

    // ((void (*)(boot_info_t*)) entry)(&gBI);
    while(1);
    return ERR(1);
}
