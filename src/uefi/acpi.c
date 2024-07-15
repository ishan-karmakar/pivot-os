#include <uefi.h>
#include <boot.h>
#include <acpi.h>
#include <util/logger.h>
#include <libc/string.h>

const efi_guid_t ACPI_20_GUID = {0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}};
const efi_guid_t ACPI_10_GUID = {0xeb9d2d30,0x2d88,0x11d3,{0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}};

static uintptr_t search_guid(const efi_guid_t *guid) {
    for (size_t i = 0; i < gST->table_entries; i++)
        if (!memcmp(guid, &gST->config_table[i].vendor_guid, sizeof(efi_guid_t)))
            return (uintptr_t) gST->config_table[i].vendor_table;
    return 0;
}

struct [[gnu::packed]] rsdp {
    char sig[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    uint8_t rsv[3];
};

void parse_rsdp(uintptr_t rsdp_addr) {
    struct rsdp *rsdp = (struct rsdp*) rsdp_addr;
    if (!memcmp(rsdp->sig, "RSD PTR ", 8))
        log(Warning, "ACPI", "RSDP signature did not match");
    
    log(Info, "")
    while(1);
}

efi_status_t init_acpi(void) {
    gBI.rsdp = search_guid(&ACPI_20_GUID);
    if (!gBI.rsdp)
        gBI.rsdp = search_guid(&ACPI_10_GUID);
    if (!gBI.rsdp) {
        log(Error, "ACPI", "Couldn't find any ACPI tables");
        return ERR(14);
    }
    log(Info, "ACPI", "Found ACPI tables");
    parse_rsdp(gBI.rsdp);
    return 0;
}
