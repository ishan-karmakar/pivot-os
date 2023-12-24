#include <boot/acpi.h>
#include <kernel/acpi.h>

INTN FindGuid(EFI_GUID *tableGuid) {
    for (UINTN i = 0; i < gST->NumberOfTableEntries; i++) {
        EFI_GUID guid = gST->ConfigurationTable[i].VendorGuid;
        if (!CompareGuid(tableGuid, &guid))
            return i;
    }
    return -1;
}

BOOLEAN ValidateTable(UINT8 *table, UINTN length) {
    UINT8 sum = 0;
    for (UINTN i = 0; i < length; i++)
        sum += table[i];
    return sum == 0;
}

EFI_STATUS FindRSDP(boot_info_t *boot_info) {
    EFI_GUID acpi20guid = ACPI_20_TABLE_GUID;
    INTN index = FindGuid(&acpi20guid);
    // index = -1; // Use this to test for RSDPv1
    UINT8 *tableBytes;
    if (index != -1)
        tableBytes = gST->ConfigurationTable[index].VendorTable;
    else {
        // Table wasn't found
        EFI_GUID acpi_guid = ACPI_TABLE_GUID;
        index = FindGuid(&acpi_guid);
        if (index == -1) {
            Print(L"Couldn't find any ACPI tables\n");
            return EFI_NOT_FOUND;
        }

        tableBytes = gST->ConfigurationTable[index].VendorTable;
    }

    if (!ValidateTable(tableBytes, 20)) {
        Print(L"RSDP was not valid\n");
        return EFI_NOT_FOUND;
    }

    rsdp_descriptor_t *rsdp = (rsdp_descriptor_t*) tableBytes;
    if (rsdp->revision == 2) {
        if (!ValidateTable(tableBytes, rsdp->length)) {
            Print(L"RSDP was not valid\n");
            return EFI_NOT_FOUND;
        }
        boot_info->sdt_address = rsdp->xsdt_address;
        boot_info->xsdt = true;
        Print(L"Found a valid RSDPv2\n");
    } else {
        boot_info->sdt_address = rsdp->rsdt_address;
        boot_info->xsdt = false;
        Print(L"Found a valid RSDPv1\n");
    }
    return EFI_SUCCESS;
}