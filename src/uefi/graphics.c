#include <types.h>
#include <uefi.h>
#include <graphics.h>

const efi_guid_t GOP_GUID = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

efi_status_t find_video_mode(efi_gop_t *gop, uint32_t target_width, uint32_t target_height, efi_graphics_pixel_format_t target_pixel_format, size_t *target_mode) {
    efi_status_t status;
    size_t mode_size;
    efi_gop_mode_info_t *mode_info;
    for (size_t i = 0; i < gop->mode->max_mode; i++) {
        status = gop->query_mode(gop, i, &mode_size, &mode_info);
        if (EFI_ERR(status)) return status;

        if (mode_info->horizontal_res == target_width && mode_info->vertical_res == target_height && mode_info->pixel_format == target_pixel_format) {
            *target_mode = i;
            return 0;
        }
    }
    return ERR(3);
}

efi_status_t init_graphics(void) {
    efi_status_t status;
    efi_gop_t *gop;
    status = gST->bs->locate_protocol(&GOP_GUID, NULL, &gop);
    if (EFI_ERR(status)) return status;

    size_t video_mode;
    status = find_video_mode(gop, 1024, 768, PixelBlueGreenRedReserved8BitPerColor, &video_mode);
    if (EFI_ERR(status)) return status;

    status = gST->con_out->clear_screen(gST->con_out);
    if (EFI_ERR(status)) return status;

    status = gop->set_mode(gop, video_mode);
    if (EFI_ERR(status)) return status;

    return 0;
}
