#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
    EFI_STATUS status = uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"Hello World\n");
    return status;
}