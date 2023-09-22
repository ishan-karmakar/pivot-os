#include <kernel/acpi.h>
#include <kernel/logging.h>
#include <mem/bitmap.h>
#include <mem/pmm.h>
#include <libc/string.h>
#include <sys.h>
extern void hcf(void);

sdt_header_t *header;
uint8_t rsdt_version;
size_t num_tables;
uint8_t validate(char *start, size_t length);
void parse_rsdp(rsdp_descriptor_t*);

char *madt_items[] = {
    "LAPIC",
    "I/O APIC",
    "Interrupt Source Override",
    "NMI",
    "LAPIC NMI",
    "LAPIC Address Override",
    // There are more, but they are probably not going to be listed
};

void init_acpi(mb_tag_t *acpi_tag) {
    log(Info, true, "ACPI", "Found %s RSDP", acpi_tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD ? "old" : "new");
    if (acpi_tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD) {
        rsdp_descriptor_t *rsdp = (rsdp_descriptor_t*)(acpi_tag + 1);
        if (validate((char*) rsdp, sizeof(*rsdp))) {
            log(Error, true, "ACPI", "Detected invalid RSDP. Halting...");
            hcf();
        } else
            log(Info, true, "ACPI", "Detected valid RSDP");
        parse_rsdp(rsdp);
    } else {
        // TODO: Fix this
    }
}

uint8_t validate(char *start, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += start[i];
    }
    return sum;
}

// Parse RSDPv1
void parse_rsdp(rsdp_descriptor_t *rsdp) {
    rsdt_version = 1;
    map_addr(ALIGN_ADDR(rsdp->rsdt_address), VADDR(rsdp->rsdt_address), WRITE_BIT | PRESENT_BIT);
    bitmap_set_bit_addr(ALIGN_ADDR(rsdp->rsdt_address));

    header = (sdt_header_t*) VADDR(rsdp->rsdt_address);
    if (validate((char*) header, header->length)) {
        log(Error, true, "ACPI", "Detected invalid RSDT. Halting...");
        hcf();
    } else
        log(Info, true, "ACPI", "Detected valid RSDT");
    
    log(Verbose, true, "ACPI", "SDT Length: %u", header->length);
    size_t pages = (header->length / PAGE_SIZE) + 1;
    for (size_t i = 1; i < pages; i++) {
        uint64_t phys_addr = rsdp->rsdt_address + i * PAGE_SIZE;
        map_addr(ALIGN_ADDR(phys_addr), VADDR(phys_addr), WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit_addr(ALIGN_ADDR(phys_addr));
    }
    num_tables = (header->length - sizeof(*header)) / sizeof(uint32_t);
    log(Verbose, true, "ACPI", "Found %u tables", num_tables);
    uint32_t *tables = (uint32_t*) (header + 1);
    for (size_t i = 0; i < num_tables; i++) {
        map_addr(ALIGN_ADDR(tables[i]), VADDR(tables[i]), WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit_addr(ALIGN_ADDR(tables[i]));
        sdt_header_t *tbl_header = (sdt_header_t*) VADDR(tables[i]);
        if (validate((char*) tbl_header, tbl_header->length)) {
            log(Error, true, "ACPI", "Detected invalid table. Halting...");
            hcf();
        }
        char signature[5];
        memcpy(signature, tbl_header->signature, 4);
        signature[4] = 0;
        log(Info, true, "ACPI", "[%u] %s", i, signature);
    }
}

// Parse RSDPv2
// void parse_rsdp2(rsdp_descriptor2_t *rsdp) {}

sdt_header_t *get_table(char *signature) {
    for (uint32_t i = 0; i < num_tables; i++) {
        sdt_header_t *table;
        if (rsdt_version == 1)
            table = (sdt_header_t*) VADDR(((uint32_t*)(header + 1))[i]);
        else
            table = (sdt_header_t*) VADDR(((uint64_t*)(header + 1))[i]);
        if (!(memcmp(table->signature, signature, 4)))
            return table;
    }
    return NULL;
}

void print_madt(madt_t *table) {
    madt_item_t *item = (madt_item_t*)(table + 1);
    size_t total_length = sizeof(madt_t);
    uint32_t i = 0;
    while (total_length < table->header.length) {
        log(Verbose, false, "MADT", "Type: %s - Length: %d", madt_items[item->type], item->length);
        total_length += item->length;
        item = (madt_item_t*)((uint64_t) item + item->length);
        i++;
    }
    flush_screen();
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
