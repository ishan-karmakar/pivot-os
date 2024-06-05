#include <boot/graphics.h>

EFI_STATUS FindVideoMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *const gop,
    const UINT32 target_width,
    const UINT32 target_height,
    const EFI_GRAPHICS_PIXEL_FORMAT target_pixel_format,
    UINTN *target_mode) {
    EFI_STATUS status;
    UINTN mode_size;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info;
    for (UINTN i = 0; i < gop->Mode->MaxMode; i++) {
        status = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &mode_size, &mode_info);
        if (EFI_ERROR(status)) {
            Print(L"Error querying video mode %u", i);
            return status;
        }

        if (mode_info->HorizontalResolution == target_width &&
            mode_info->VerticalResolution == target_height &&
            mode_info->PixelFormat == target_pixel_format) {
            *target_mode = i;
            return EFI_SUCCESS;
        }
    }
    Print(L"Couldn't find a supported video mode\n");
    return EFI_UNSUPPORTED;
}

EFI_STATUS ConfigureGraphics(kernel_info_t *kinfo) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = uefi_call_wrapper(gBS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, &gop);
    if (EFI_ERROR(status)) {
        Print(L"Error locating graphics output protocol\n");
        return status;
    }

    UINTN video_mode;
    status = FindVideoMode(gop, TARGET_SCREEN_WIDTH, TARGET_SCREEN_HEIGHT, TARGET_PIXEL_FORMAT, &video_mode);
    if (EFI_ERROR(status))
        return status;
    
    status = uefi_call_wrapper(gST->ConOut->ClearScreen, 1, gST->ConOut);
    if (EFI_ERROR(status)) {
        Print(L"Error clearing screen\n");
        return status;
    }

    status = uefi_call_wrapper(gop->SetMode, 2, gop, video_mode);
    if (EFI_ERROR(status)) {
        Print(L"Error setting graphics mode\n");
        return status;
    }
    Print(L"Set graphics mode...\n");

    kinfo->fb.buffer = (char*) gop->Mode->FrameBufferBase;
    kinfo->fb.horizontal_res = gop->Mode->Info->HorizontalResolution;
    kinfo->fb.vertical_res = gop->Mode->Info->VerticalResolution;
    kinfo->fb.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    kinfo->fb.bpp = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    return EFI_SUCCESS;
}

EFI_STATUS CloseGraphics(EFI_HANDLE **handle_buffer) {
    EFI_STATUS status;
    status = uefi_call_wrapper(gBS->FreePool, 1, *handle_buffer);
    if (EFI_ERROR(status)) {
        Print(L"Error closing graphics output service\n");
        return status;
    }

    return EFI_SUCCESS;
}