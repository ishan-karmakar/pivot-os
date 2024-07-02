#include <graphics.h>
#include <types.h>
#include <util/logger.h>

const efi_guid_t GOP_GUID = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

efi_status_t init_graphics(efi_system_table_t *st) {
    efi_status_t status;
    efi_gop_t *gop;

    status = st->boot_services->locate_protocol(&GOP_GUID, NULL, (void**) &gop);
    if (status < 0) return status;

    size_t target_idx = 0;
    efi_gop_mode_info_t *target_mode = gop->mode->info;
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

// Since QEMU is an emulator, it offers many different aspect ratios and sizes
// but I want something that is smaller than the screen so I can use the toolbar
#ifndef QEMU
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
        return -3;
    }
    gop->set_mode(gop, target_idx);
    status = st->con_out->clear_screen(st->con_out);
    if (status < 0) return status;

    log(Info, "GOP", "Chose video mode with resolution %ux%u", target_mode->horizontal_res, target_mode->vertical_res);
    return 0;
}
