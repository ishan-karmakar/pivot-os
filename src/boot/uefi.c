#include <boot/graphics.h>
#include <boot/mem.h>
#include <boot/loader.h>
#include <boot/acpi.h>
#include <boot.h>
#include <sys.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS kernel_entry_point = 0;
    boot_info_t boot_info;
    UINTN mmap_key = 0;

    InitializeLib(ImageHandle, SystemTable);

    status = uefi_call_wrapper(gBS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
    if (EFI_ERROR(status)) {
        Print(L"Error setting watchdog timer\n");
        return status;
    }
    Print(L"Disabled watchdog timer...\n");

    status = uefi_call_wrapper(gST->ConIn->Reset, 2, gST->ConIn, FALSE);
    if (EFI_ERROR(status)) {
        Print(L"Error resetting console input");
        return status;
    }
    Print(L"Reset console input...\n");

    status = ConfigureGraphics(&boot_info.fb_info);
    if (EFI_ERROR(status))
        return status;
    
    status = FindRSDP(&boot_info);
    if (EFI_ERROR(status))
        return status;

    status = ConfigurePaging(&boot_info.mem_info);
    if (EFI_ERROR(status))
        return status;

    status = LoadKernel(&boot_info.mem_info, &kernel_entry_point);
    if (EFI_ERROR(status))
        return status;

    status = GetMMAP(&boot_info.mem_info, &mmap_key);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, mmap_key);
    if (EFI_ERROR(status)) {
        Print(L"Error exiting boot services\n");
        return status;
    }

    LoadCr3(&boot_info.mem_info);

    VOID (*kernel_entry)(boot_info_t*) = (VOID (*)(boot_info_t*)) kernel_entry_point;
    kernel_entry(&boot_info);
    return EFI_LOAD_ERROR;
}