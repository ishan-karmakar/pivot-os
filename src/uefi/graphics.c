#include <graphics.h>
#include <types.h>
#include <util/logger.h>

const efi_guid_t GOP_GUID = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

efi_status_t init_graphics(void) {
    efi_status_t status;
    efi_gop_t *gop;

    status = gST->boot_services->locate_protocol(&GOP_GUID, NULL, (void**) &gop);
    if (EFI_ERR(status)) return status;

    size_t target_idx = 0;
    efi_gop_mode_info_t *target_mode = gop->mode->info;
    for (size_t i = 0; i < gop->mode->max_mode; i++) {
        size_t mode_size;
        efi_gop_mode_info_t *mode_info;
        status = gop->query_mode(gop, i, &mode_size, &mode_info);
        if (EFI_ERR(status)) return status;

// Since QEMU is an emulator, it offers many different aspect ratios and sizes
// but I want something that is smaller than the screen so I can use the toolbar
// For right now, I am using a constant 1024 x 768
#ifndef DEBUG
        if (mode_info->horizontal_res > target_mode->horizontal_res) {
#else
        if (mode_info->horizontal_res == 1024 && mode_info->vertical_res == 768) {
#endif
            target_mode = mode_info;
            target_idx = i;
        }
    }
    if (target_mode == NULL) {
        log(Error, "GOP", "Couldn't find a compatible video mode");
        return ERR(3);
    }
    gop->set_mode(gop, target_idx);
    status = gST->con_out->clear_screen(gST->con_out);
    if (EFI_ERR(status)) return status;

    log(Info, "GOP", "Chose video mode with resolution %ux%u", target_mode->horizontal_res, target_mode->vertical_res);
    return 0;
}
