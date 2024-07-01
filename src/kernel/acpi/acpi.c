#include <acpi/acpi.h>
#include <util/logger.h>
#include <libc/string.h>
#include <cpu/ioapic.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <acpi/madt.h>
#include <kernel.h>

static bool validate_checksum(char*, size_t);
void parse_table(sdt_header_t*);

// At the end of this function the ACPI tables are no longer safe to access (reclaimed by PMM)
void init_acpi(void) {
    bool xsdt = KACPI.xsdt;
    xsdt_t *xsdt_tbl = (xsdt_t*) KACPI.sdt_addr;
    rsdt_t *rsdt_tbl = (rsdt_t*) KACPI.sdt_addr;
    
    uint32_t header_length = xsdt ? xsdt_tbl->header.length : rsdt_tbl->header.length;
    if (!validate_checksum(xsdt ? (char*) xsdt_tbl : (char*) rsdt_tbl, header_length))
        return log(Error, "ACPI", "Found invalid %cSDT table", xsdt ? 'X' : 'R');
    log(Verbose, "ACPI", "Found valid %cSDT table", xsdt ? 'X' : 'R');
    KACPI.num_tables = xsdt ? ((xsdt_tbl->header.length - sizeof(xsdt_tbl->header)) / sizeof(xsdt_tbl->tables[0])) : ((rsdt_tbl->header.length - sizeof(rsdt_tbl->header)) / sizeof(rsdt_tbl->tables[0]));
    char signature[5];
    signature[4] = 0;
    for (size_t i = 0; i < KACPI.num_tables; i++) {
        uintptr_t header_addr = xsdt ? xsdt_tbl->tables[i] : rsdt_tbl->tables[i];
        sdt_header_t *header = (sdt_header_t*) header_addr;
        if (!validate_checksum((char*) header, header->length))
            return log(Error, "ACPI", "Detected invalid table");
        parse_table(header);
        memcpy(signature, header->signature, 4);
        log(Verbose, "ACPI", "[%u] %s", i, signature);
    }
    log(Info, "ACPI", "Parsed ACPI tables");
    mmap_desc_t *desc = KMEM.mmap;
    for (size_t i = 0; i < (KMEM.mmap_size / KMEM.mmap_desc_size); i++, desc = (mmap_desc_t*)((char*)desc + KMEM.mmap_desc_size))
        if (desc->type == 9)
            break;
    if (desc->type == 9) {
        pmm_clear_area(desc->physical_start, desc->count);
    } else {
        log(Warning, "ACPI", "Couldn't find a MMAP region for the ACPI tables");
    }
}

void parse_table(sdt_header_t *header) {
    if (!memcmp(header->signature, "APIC", 4))
        parse_madt(header);
}

static bool validate_checksum(char *start, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++)
        sum += start[i];
    return sum == 0;
}
