#include <kernel/acpi.h>
#include <kernel/logging.h>
#include <mem/mem.h>
#include <libc/string.h>
extern void hcf(void);

sdt_header_t *header;
uint8_t validate(char *start, size_t length);
void parse_rsdp(rsdp_descriptor_t*);

void init_acpi(mb_tag_t *acpi_tag) {
    log(Info, "ACPI", "Found %s RSDP", acpi_tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD ? "old" : "new");
    if (acpi_tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD) {
        rsdp_descriptor_t *rsdp = (rsdp_descriptor_t*)(acpi_tag + 1);
        if (validate((char*) rsdp, sizeof(*rsdp))) {
            log(Error, "ACPI", "Detected invalid RSDP. Halting...");
            hcf();
        } else
            log(Info, "ACPI", "Detected valid RSDP");
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
    map_addr(ALIGN_ADDR(rsdp->rsdt_address), MAKE_HIGHER_HALF(rsdp->rsdt_address), WRITE_BIT | PRESENT_BIT);
    bitmap_set_bit(ALIGN_ADDR(rsdp->rsdt_address));

    header = (sdt_header_t*) MAKE_HIGHER_HALF(rsdp->rsdt_address);
    if (validate((char*) header, header->length)) {
        log(Error, "ACPI", "Detected invalid RSDT. Halting...");
        hcf();
    } else
        log(Info, "ACPI", "Detected valid RSDT");
    
    log(Verbose, "ACPI", "SDT Length: %u", header->length);
    size_t pages = (header->length / PAGE_SIZE) + 1;
    for (size_t i = 1; i < pages; i++) {
        uint64_t phys_addr = rsdp->rsdt_address + i * PAGE_SIZE;
        map_addr(ALIGN_ADDR(phys_addr), MAKE_HIGHER_HALF(phys_addr), WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit(ALIGN_ADDR(phys_addr));
    }
    size_t num_tables = (header->length - sizeof(*header)) / sizeof(uint32_t);
    log(Verbose, "ACPI", "Found %u tables", num_tables);
    uint32_t *tables = (uint32_t*) (header + 1);
    for (size_t i = 0; i < num_tables; i++) {
        map_addr(ALIGN_ADDR(tables[i]), MAKE_HIGHER_HALF(tables[i]), WRITE_BIT | PRESENT_BIT);
        bitmap_set_bit(ALIGN_ADDR(tables[i]));
        sdt_header_t *tbl_header = (sdt_header_t*) MAKE_HIGHER_HALF(tables[i]);
        if (validate((char*) tbl_header, tbl_header->length)) {
            log(Error, "ACPI", "Detected invalid table. Halting...");
            hcf();
        }
        char signature[5];
        memcpy(signature, tbl_header->signature, 4);
        signature[4] = 0;
        log(Info, "ACPI", "[%u] %s", i, signature);
    }
}

// Parse RSDPv2
// void parse_rsdp2(rsdp_descriptor2_t *rsdp) {}