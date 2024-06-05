#pragma once
#include <efi.h>
#include <efilib.h>
#include <kernel.h>
#define TARGET_SCREEN_WIDTH 1024
#define TARGET_SCREEN_HEIGHT 768
#define TARGET_PIXEL_FORMAT PixelBlueGreenRedReserved8BitPerColor

EFI_STATUS ConfigureGraphics(kernel_info_t *kinfo);
EFI_STATUS CloseGraphics(EFI_HANDLE **handle_buffer);
