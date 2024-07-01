#include <uefi.h>
#include <types.h>
#include <con.h>
#include <graphics.h>
#include <util/logger.h>
#include <libc/string.h>
#include <stdint.h>
#include <stdarg.h>

uint8_t CPU = 0;
efi_guid_t simple_pointer_protocol = {0x31878c87,0xb75,0x11d5, {0x9a,0x4f,0x00,0x90,0x27,0x3f,0xc1,0x4d}};

typedef struct {
    uint64_t resx;
    uint64_t resy;
    uint64_t resz;
    bool left_button;
    bool right_button;
} efi_sp_mode_t; 

typedef struct {
    int32_t relative_movement_x;
    int32_t relative_movement_y;
    int32_t relative_movement_z;
    bool left_button;
    bool right_button;
} efi_sp_state_t;

typedef struct efi_spp {
    efi_status_t (*reset)(struct efi_spp*, bool);
    efi_status_t (*get_state)(struct efi_spp*, efi_sp_state_t*);
    uintptr_t wait_for_input;
    efi_sp_mode_t *mode;
} efi_spp_t;

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

    status = init_graphics(st->boot_services);
    if (status < 0) return status;
    while (1);
}
