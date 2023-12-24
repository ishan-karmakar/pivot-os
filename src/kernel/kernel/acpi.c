#include <kernel/acpi.h>
#include <kernel/logging.h>
#include <libc/string.h>
#include <mem/pmm.h>

extern void hcf(void);
static bool validate_checksum(char*, size_t);
static void validate_tables(void);

bool xsdt;
uintptr_t sdt_addr;
size_t num_tables;

void init_acpi(boot_info_t *boot_info) {
    map_addr(boot_info->sdt_address, VADDR(boot_info->sdt_address), PAGE_TABLE_ENTRY);
    sdt_addr = boot_info->sdt_address;
    xsdt = boot_info->xsdt;
    validate_tables();
}

static void validate_tables(void) {
    xsdt_t *xsdt_tbl = (xsdt_t*) sdt_addr;
    rsdt_t *rsdt_tbl = (rsdt_t*) sdt_addr;
    map_range(sdt_addr, VADDR(sdt_addr), SIZE_TO_PAGES(xsdt ? xsdt_tbl->header.length : rsdt_tbl->header.length));
    bitmap_rsv_area(sdt_addr, SIZE_TO_PAGES(xsdt ? xsdt_tbl->header.length : rsdt_tbl->header.length));

    if (xsdt) {
        if (!validate_checksum((char*) xsdt_tbl, xsdt_tbl->header.length)) {
            log(Error, "ACPI", "Found invalid XSDT table");
            hcf();
        }
    } else {
        if (!validate_checksum((char*) rsdt_tbl, rsdt_tbl->header.length)) {
            log(Error, "ACPI", "Found invalid RSDT table");
            hcf();
        }
    }
    log(Verbose, "ACPI", "Found valid %s table", xsdt ? "XSDT" : "RSDT");
    num_tables = xsdt ? ((xsdt_tbl->header.length - sizeof(xsdt_tbl->header)) / sizeof(xsdt_tbl->tables[0])) :
                 ((rsdt_tbl->header.length - sizeof(rsdt_tbl->header)) / sizeof(rsdt_tbl->tables[0]));
    log(Verbose, "ACPI", "Found %u tables", num_tables);
    char signature[5];
    for (size_t i = 0; i < num_tables; i++) {
        if (xsdt)
            map_addr(xsdt_tbl->tables[i], VADDR(xsdt_tbl->tables[i]), PAGE_TABLE_ENTRY);
        else
            map_addr(rsdt_tbl->tables[i], VADDR(rsdt_tbl->tables[i]), PAGE_TABLE_ENTRY);
        uintptr_t header_addr = xsdt ? xsdt_tbl->tables[i] : rsdt_tbl->tables[i];
        sdt_header_t *header = (sdt_header_t*) VADDR(header_addr);
        if (!validate_checksum((char*) header, header->length)) {
            log(Error, "ACPI", "Detected invalid table...");
            hcf();
        }
        map_range(header_addr, VADDR(header_addr), SIZE_TO_PAGES(header->length));
        bitmap_rsv_area(header_addr, SIZE_TO_PAGES(header->length));
        memcpy(signature, header->signature, 4);
        signature[4] = 0;
        log(Info, "ACPI", "[%u] %s", i, signature);
    }
}

static bool validate_checksum(char *start, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++)
        sum += start[i];
    return sum == 0;
}

sdt_header_t *get_table(char *signature) {
    xsdt_t *xsdt_tbl = (xsdt_t*) sdt_addr;
    rsdt_t *rsdt_tbl = (rsdt_t*) sdt_addr;
    for (size_t i = 0; i < num_tables; i++) {
        sdt_header_t *table;
        if (xsdt)
            table = (sdt_header_t*) VADDR(xsdt_tbl->tables[i]);
        else
            table = (sdt_header_t*) VADDR(rsdt_tbl->tables[i]);
        
        if (!memcmp(table->signature, signature, 4))
            return table;
    }
    return NULL;
}

madt_item_t *get_madt_item(madt_t *table, uint8_t search_item, uint8_t count) {
    madt_item_t *item = (madt_item_t*)(table + 1);
    uint8_t counter = 0;
    for (size_t total_length = sizeof(madt_t); total_length < table->header.length; total_length += item->length, item = (madt_item_t*)((uint64_t) item + item->length))
        if (item->type == search_item) {
            if (counter == count)
                return item;
            else
                counter++;
        }
    return NULL;
}