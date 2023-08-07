#include <stddef.h>
#include <cpu/ioapic.h>
#include <kernel/logging.h>
#include <mem/mem.h>
#include <libc/string.h>

static uint32_t read_register(uint8_t);
static int read_redirect(uint8_t, ioapic_redtbl_entry*);
static uint32_t parse_interrupt_so(madt_t*);

uint64_t ioapic_base_address;
ioapic_source_override_t ioapic_so[IOAPIC_MAX_SOURCE_OVERRIDE];
uint32_t ioapic_so_size;

void init_ioapic(madt_t* table) {
    madt_item_t *item = (madt_item_t*) get_madt_item(table, MADT_IOAPIC, 0);
    if (item == NULL)
        return log(Error, "IOAPIC", "Couldn't find IOAPIC in MADT table");
    ioapic_t *ioapic = (ioapic_t*) (item + 1);
    map_addr(ALIGN_ADDR(MAKE_PHYS_ADDR((uint64_t) ioapic)), (uint64_t) ioapic, WRITE_BIT | PRESENT_BIT);
    log(Verbose, "IOAPIC", "I/O APIC ID: %u, Address: %x, GSI Base: %u", ioapic->id, ioapic->addr, ioapic->gsi_base);
    ioapic_base_address = MAKE_HIGHER_HALF(ioapic->addr);
    map_addr(ioapic->addr, ioapic_base_address, PRESENT_BIT | WRITE_BIT);
    uint32_t ioapic_version = read_register(IOAPIC_VER_OFFSET);
    log(Info, "IOAPIC", "IOAPIC Version: %x", ioapic_version);
    ioapic_redtbl_entry entry;
    if (read_redirect(0x10, &entry) != 0)
        return log(Error, "IOAPIC", "Error reading redirection entry");
    uint8_t ioapic_max_redirections = (uint8_t) (ioapic_version >> 16);
    log(Verbose, "IOAPIC", "Max redirection entries: %d", ioapic_max_redirections);
    ioapic_so_size = parse_interrupt_so(table);
    log(Verbose, "IOAPIC", "Found %u source override entries", ioapic_so_size);
}

static uint32_t read_register(uint8_t offset) {
    *(volatile uint32_t*) ioapic_base_address = offset;
    return *(volatile uint32_t*)(ioapic_base_address + 0x10);
}

static int read_redirect(uint8_t index, ioapic_redtbl_entry *entry) {
    if (index < 0x10 || index > 0x3F || index % 2)
        return -1;
    uint32_t lower_part = read_register(index);
    uint32_t upper_part = read_register(index + 1);
    entry->raw = ((uint64_t) upper_part << 32) | (uint64_t) lower_part;
    return 0;
}

static uint32_t parse_interrupt_so(madt_t *table) {
    uint32_t counter = 0;
    madt_item_t *item = get_madt_item(table, MADT_INTERRUPT_SOURCE_OVERRIDE, 0);
    while (item != NULL) {
        memcpy(&ioapic_so[counter], item + 1, sizeof(ioapic_source_override_t));
        log(Verbose, "IOAPIC", "SO Item: bus source: %u - irq source: %u - GSI Base: %d",
            ioapic_so[counter].bus_source, ioapic_so[counter].irq_source, ioapic_so[counter].gsi_base);
        counter++;
        item = get_madt_item(table, MADT_INTERRUPT_SOURCE_OVERRIDE, counter);
    }
    return counter;
}
