#include <graphics.h>
#include <types.h>
#include <util/logger.h>
#define TARGET_HRES 1280
#define TARGET_VRES 768

efi_guid_t gop_guid = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };
efi_guid_t edid_discovered_guid = {0xbd8c1056,0x9f36,0x44ec, {0x92,0xa8,0xa6,0x33,0x7f,0x81,0x79,0x86}};

efi_status_t init_graphics(efi_boot_services_t *bs) {
    efi_status_t status;
    efi_gop_t *gop;

    status = bs->locate_protocol(&gop_guid, NULL, (void**) &gop);
    if (status < 0) return status;

    size_t target_idx;
    efi_gop_mode_info_t *target_mode = NULL;
    for (size_t i = 0; i < gop->mode->max_mode; i++) {
        size_t mode_size;
        efi_gop_mode_info_t *mode_info;
        status = gop->query_mode(gop, i, &mode_size, &mode_info);
        if (status < 0) return status;
        log(Verbose, "GOP", "[%u] %ux%u, %u",
            i,
            mode_info->horizontal_res,
            mode_info->vertical_res,
            mode_info->pixel_format);
        if (mode_info->horizontal_res == TARGET_HRES && mode_info->vertical_res == TARGET_VRES) {
            target_mode = mode_info;
            target_idx = i;
            break;
        }
    }
    if (target_mode == NULL) {
        log(Error, "GOP", "Couldn't find a compatible video mode");
        return -3;
    }
    log(Info, "GOP", "Choosing mode with resolution %ux%u", target_mode->horizontal_res, target_mode->vertical_res);
    gop->set_mode(gop, target_idx);
    return 0;
}
