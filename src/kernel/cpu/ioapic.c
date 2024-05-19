#include <stddef.h>
#include <cpu/ioapic.h>
#include <kernel/logging.h>
#include <kernel/acpi.h>
#include <mem/pmm.h>
#include <libc/string.h>
#include <sys.h>

static uint32_t read_register(uint8_t);
static int read_redirect(uint8_t, ioapic_redtbl_entry_t*);

uint64_t ioapic_base_address;

void init_ioapic(void) {
    madt_t *table = (madt_t*) get_table("APIC");
    ioapic_t *ioapic = (ioapic_t*) get_madt_item(table, MADT_IOAPIC, 0);
    if (ioapic == NULL)
        return log(Error, "IOAPIC", "Couldn't find IOAPIC in MADT table");
    map_addr((uintptr_t) ioapic, (uintptr_t) ioapic, KERNEL_PT_ENTRY, NULL);
    log(Verbose, "IOAPIC", "ID: %u, Address: %x, GSI Base: %u", ioapic->id, ioapic->addr, ioapic->gsi_base);
    ioapic_base_address = ioapic->addr;
    // map_addr(ioapic->addr, ioapic_base_address, KERNEL_PT_ENTRY, NULL);
    uint32_t ioapic_version = read_register(IOAPIC_VER_OFFSET);
    ioapic_redtbl_entry_t entry;
    if (read_redirect(0x10, &entry) != 0)
        return log(Error, "IOAPIC", "Error reading redirection entry");
    uint8_t ioapic_max_redirections = (uint8_t) (ioapic_version >> 16);
    log(Verbose, "IOAPIC", "Max redirection entries: %d", ioapic_max_redirections);
    log(Info, "IOAPIC", "Initialized IOAPIC");
}

static uint32_t read_register(uint8_t offset) {
    *(volatile uint32_t*) ioapic_base_address = offset;
    return *(volatile uint32_t*)(ioapic_base_address + 0x10);
}

static void write_register(uint8_t offset, uint32_t value) {
    *(volatile uint32_t*) ioapic_base_address = offset;
    *(volatile uint32_t*) (ioapic_base_address + 0x10) = value;
}

static int read_redirect(uint8_t index, ioapic_redtbl_entry_t *entry) {
    if (index < 0x10 || index > 0x3F || index % 2)
        return -1;
    uint32_t lower_part = read_register(index);
    uint32_t upper_part = read_register(index + 1);
    entry->raw = ((uint64_t) upper_part << 32) | (uint64_t) lower_part;
    return 0;
}

static int write_redirect(uint8_t index, ioapic_redtbl_entry_t entry) {
    if (index < 0x10 || index > 0x3F || index % 2)
        return -1;
    uint32_t upper_part = (uint32_t) (entry.raw >> 32);
    uint32_t lower_part = (uint32_t) entry.raw;
    write_register(index, lower_part);
    write_register(index + 1, upper_part);
    return 0;
}

static ioapic_so_t *find_so(uint8_t irq) {
    size_t count = 0;
    madt_t *madt = (madt_t*) get_table("APIC");
    ioapic_so_t *item = (ioapic_so_t*) get_madt_item(madt, MADT_INT_SO_OVRD, 0);
    while (item != NULL) {
        if (item->irq_source == irq)
            return item;
        item = (ioapic_so_t*) get_madt_item(madt, MADT_INT_SO_OVRD, ++count);
    }
    return NULL;
}

void set_irq(uint8_t irq, uint8_t idt_entry, uint8_t destination_field, uint32_t flags, bool masked) {
    uint8_t pin = irq;
    ioapic_redtbl_entry_t entry;
    entry.raw = flags | idt_entry;
    entry.destination = destination_field;
    entry.mask = masked;
    ioapic_so_t *so = find_so(irq);
    if (so) {
        pin = so->gsi_base;
        entry.pin_polarity = so->flags & 2;
        entry.trigger_mode = so->flags & 8;
    }

    uint8_t redtbl_pos = 0x10 + pin * 2;
    log(Verbose, "IOAPIC", "Setting IRQ %u to idt entry %u at REDTBL pos: %x",
        pin, idt_entry, redtbl_pos);
    if (write_redirect(redtbl_pos, entry))
        log(Error, "IOAPIC", "Error writing to redirection table");
}

void set_irq_mask(uint8_t irq, bool masked) {
    ioapic_so_t *so = find_so(irq);
    if (so)
        irq = so->gsi_base;
    uint8_t redtbl_pos = 0x10 + irq * 2;
    ioapic_redtbl_entry_t entry;
    read_redirect(redtbl_pos, &entry);
    entry.mask = masked;
    write_redirect(redtbl_pos, entry);
}