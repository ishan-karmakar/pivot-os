#pragma once
#include <efi.h>
#include <efilib.h>
#include <boot.h>
#define TARGET_SCREEN_WIDTH 1024
#define TARGET_SCREEN_HEIGHT 768
#define TARGET_PIXEL_FORMAT PixelBlueGreenRedReserved8BitPerColor

EFI_STATUS ConfigureGraphics(boot_info_t *boot_info);
EFI_STATUS CloseGraphics(EFI_HANDLE **handle_buffer);
