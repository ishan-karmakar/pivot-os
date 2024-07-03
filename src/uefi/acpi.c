#include <acpi.h>
#include <util/logger.h>
#include <libc/string.h>

const efi_guid_t ACPI_20_GUID = {0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}};
const efi_guid_t ACPI_10_GUID = {0xeb9d2d30,0x2d88,0x11d3,{0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}};

static void *search_guid(const efi_system_table_t *st, const efi_guid_t *guid) {
    for (size_t i = 0; i < st->table_entries; i++)
        if (!memcmp(guid, &st->config_table[i].vendor_guid, sizeof(efi_guid_t)))
            return st->config_table[i].vendor_table;
    return NULL;
}

void init_acpi(efi_system_table_t *st) {
    void *acpi_20 = search_guid(st, &ACPI_20_GUID);
    if (!acpi_20) {
        // void *acpi_10 = search_guid(st, &ACPI_10_GUID);
        log(Info, "ACPI", "Found ACPI 1.0 tables");
    } else {
        log(Info, "ACPI", "Found ACPI 2.0 tables");
    }
}
