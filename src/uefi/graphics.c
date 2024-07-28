#include <uefi.h>
#include <graphics.h>
#include <util/logger.hpp>
#include <con.h>
#include <boot.h>

const efi_guid_t GOP_GUID = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

efi_status_t init_graphics(void) {
    efi_status_t status;
    efi_gop_t *gop;
    status = gST->bs->locate_protocol(&GOP_GUID, NULL, (void**) &gop);
    if (EFI_ERR(status)) return status;

    size_t video_mode;
    efi_gop_mode_info_t *tmi;
    for (size_t i = 0; i < gop->mode->max_mode; i++) {
        efi_gop_mode_info_t *mi;
        size_t mode_size;
        status = gop->query_mode(gop, i, &mode_size, &mi);
        if (EFI_ERR(status)) return status;

#ifdef DEBUG
        if (mi->horizontal_res == 1024 && mi->vertical_res == 768) {
#else
        if (mi->horizontal_res > tmi->horizontal_res) {
#endif
            tmi = mi;
            video_mode = i;
        }
    }

    status = gST->con_out->clear_screen(gST->con_out);
    if (EFI_ERR(status)) return status;

    status = gop->set_mode(gop, video_mode);
    if (EFI_ERR(status)) return status;

    log(Info, "GRAPHICS", "Set graphics mode to %ux%u", tmi->horizontal_res, tmi->vertical_res);
    gBI.fb_buf = gop->mode->fb_base;
    gBI.hres = tmi->horizontal_res;
    gBI.vres = tmi->vertical_res;
    gBI.pps = tmi->pp_scan_line;
    return 0;
}
