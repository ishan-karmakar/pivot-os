#include <uefi.h>
#include <con.h>
#include <graphics.h>
#include <acpi.h>
#include <util/logger.h>
#include <libc/string.h>
#include <stdint.h>
#include <stdarg.h>

uint8_t CPU = 0;

efi_status_t verify_table(efi_boot_services_t *bs, efi_table_header_t *header) {
    uint32_t crc32, old_crc32 = header->crc32;
    header->crc32 = 0;
    efi_status_t status = bs->calculate_crc32(header, header->header_size, &crc32);
    if (status < 0) return status;
    if (crc32 == old_crc32)
        return 0;
    else
        return -27;
}

efi_status_t efi_main(void *image_handle, efi_system_table_t *st) {
    efi_status_t status;
    status = init_con(st->con_out);
    if (status < 0) return status;
    
    status = verify_table(st->boot_services, &st->header);
    if (status < 0) return status;
    log(Info, "EFI", "System table verified");

    status = verify_table(st->boot_services, &st->boot_services->header);
    if (status < 0) return status;
    log(Info, "EFI", "Boot services table verified");

    status = init_graphics(st);
    if (status < 0) return status;

    init_acpi(st);
    while (1);
}
