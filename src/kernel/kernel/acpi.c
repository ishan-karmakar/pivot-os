#include <kernel/acpi.h>
#include <kernel/logging.h>
#include <libc/string.h>
#include <mem/bitmap.h>
#include <mem/pmm.h>

extern void hcf(void);
static bool validate_checksum(char*, size_t);
static void validate_tables(void);

bool xsdt;
uintptr_t sdt_addr;
size_t num_tables;

void init_acpi(boot_info_t *boot_info) {
    bitmap_set_bit(boot_info->sdt_address);
    sdt_addr = boot_info->sdt_address;
    map_addr(boot_info->sdt_address, sdt_addr, KERNEL_PT_ENTRY, NULL);
    xsdt = boot_info->xsdt;
    validate_tables();
    log(Info, "ACPI", "Initialized ACPI tables");
}

static void validate_tables(void) {
    xsdt_t *xsdt_tbl = (xsdt_t*) sdt_addr;
    rsdt_t *rsdt_tbl = (rsdt_t*) sdt_addr;
    
    uint32_t header_length = xsdt ? xsdt_tbl->header.length : rsdt_tbl->header.length;
    size_t num_pages = SIZE_TO_PAGES(sdt_addr - ALIGN_ADDR(sdt_addr) + header_length);
    bitmap_rsv_area(sdt_addr, num_pages);
    map_range(sdt_addr, sdt_addr, num_pages, KERNEL_PT_ENTRY, NULL);
    if (!validate_checksum(xsdt ? (char*) xsdt_tbl : (char*) rsdt_tbl, header_length)) {
        log(Error, "ACPI", "Found invalid %cSDT table", xsdt ? 'X' : 'R');
        hcf();
    }
    log(Verbose, "ACPI", "Found valid %cSDT table", xsdt ? 'X' : 'R');
    num_tables = xsdt ? ((xsdt_tbl->header.length - sizeof(xsdt_tbl->header)) / sizeof(xsdt_tbl->tables[0])) : ((rsdt_tbl->header.length - sizeof(rsdt_tbl->header)) / sizeof(rsdt_tbl->tables[0]));
    char signature[5];
    for (size_t i = 0; i < num_tables; i++) {
        if (xsdt)
            map_addr(xsdt_tbl->tables[i], xsdt_tbl->tables[i], KERNEL_PT_ENTRY, NULL);
        else
            map_addr(rsdt_tbl->tables[i], rsdt_tbl->tables[i], KERNEL_PT_ENTRY, NULL);
        uintptr_t header_addr = xsdt ? xsdt_tbl->tables[i] : rsdt_tbl->tables[i];
        sdt_header_t *header = (sdt_header_t*) header_addr;
        if (!validate_checksum((char*) header, header->length)) {
            log(Error, "ACPI", "Detected invalid table");
            hcf();
        }
        map_range(header_addr, header_addr, SIZE_TO_PAGES(header->length), KERNEL_PT_ENTRY, NULL);
        bitmap_rsv_area(header_addr, SIZE_TO_PAGES(header->length));
        memcpy(signature, header->signature, 4);
        signature[4] = 0;
        log(Verbose, "ACPI", "[%u] %s", i, signature);
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
            table = (sdt_header_t*) xsdt_tbl->tables[i];
        else
            table = (sdt_header_t*)(uintptr_t) rsdt_tbl->tables[i];
        
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