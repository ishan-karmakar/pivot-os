#include <boot/graphics.h>
#include <boot/mem.h>
#include <boot/loader.h>
#include <boot/acpi.h>
#include <kernel.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS kernel_entry_point = 0;
    kernel_info_t kinfo;
    EFI_PHYSICAL_ADDRESS stack;
    UINTN mmap_key = 0;

    InitializeLib(ImageHandle, SystemTable);

    status = uefi_call_wrapper(gBS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
    if (EFI_ERROR(status)) {
        Print(L"Error setting watchdog timer\n");
        return status;
    }
    Print(L"Disabled watchdog timer...\n");

    status = uefi_call_wrapper(gBS->SetMem, 3, &kinfo, sizeof(kernel_info_t), 0);
    if (EFI_ERROR(status)) {
        Print(L"Error zeroing kernel info struct\n");
        return status;
    }

    status = uefi_call_wrapper(gST->ConIn->Reset, 2, gST->ConIn, FALSE);
    if (EFI_ERROR(status)) {
        Print(L"Error resetting console input");
        return status;
    }
    Print(L"Reset console input...\n");

    status = ConfigureGraphics(&kinfo);
    if (EFI_ERROR(status))
        return status;
    
    status = FindRSDP(&kinfo);
    if (EFI_ERROR(status))
        return status;

    status = ConfigurePaging(&kinfo);
    if (EFI_ERROR(status))
        return status;

    status = LoadKernel(&kinfo, &kernel_entry_point);
    if (EFI_ERROR(status))
        return status;
    
    status = uefi_call_wrapper(gBS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, &stack);
    if (EFI_ERROR(status))
        return status;
    Print(L"Allocated stack\n");

    status = MapRange(stack, VADDR(stack), 1, kinfo.mem.pml4);
    if (EFI_ERROR(status))
        return status;

    stack += PAGE_SIZE;

    status = GetMMAP(&kinfo, &mmap_key);
    if (EFI_ERROR(status))
        return status;

    status = ParseMMAP(&kinfo);
    if (EFI_ERROR(status))
        return status;

    status = FreeMMAP(&kinfo);
    if (EFI_ERROR(status))
        return status;
    
    status = GetMMAP(&kinfo, &mmap_key);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, mmap_key);
    if (EFI_ERROR(status)) {
        Print(L"Error exiting boot services\n");
        return status;
    }
    KMEM.pml4 = (UINT64*) VADDR(KMEM.pml4);

    asm volatile (
        "mov %0, %%cr3\n"
        "mov %1, %%rsp\n"
        : : "r" (PADDR(kinfo.mem.pml4)), "g" (VADDR(stack)));
    VOID (*kernel_entry)(kernel_info_t*, uintptr_t) = (VOID (*)(kernel_info_t*, uintptr_t)) kernel_entry_point;
    kernel_entry(&kinfo, stack - PAGE_SIZE);
    return EFI_LOAD_ERROR;
}
