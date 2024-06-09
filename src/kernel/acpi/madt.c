#include <acpi/madt.h>
#include <stddef.h>
#include <kernel.h>
#include <mem/heap.h>
#include <cpu/ioapic.h>
#include <libc/string.h>

size_t get_madt_count(madt_t*, uint8_t);
madt_item_t *get_madt_item(madt_t*, uint8_t, uint8_t);

void parse_madt(sdt_header_t *header) {
    madt_t *madt = (madt_t*) header;
    KLAPIC.addr = madt->lapic_base;

    ioapic_t *ioapic = (ioapic_t*) get_madt_item(madt, MADT_IOAPIC, 0);
    KIOAPIC.addr = ioapic->addr;
    KIOAPIC.gsi_base = ioapic->gsi_base;
    KIOAPIC.num_ovrds = get_madt_count(madt, MADT_INT_SO_OVRD);
    KIOAPIC.ovrds = halloc(sizeof(ioapic_so_t) * KIOAPIC.num_ovrds, KHEAP);
    for (size_t i = 0; i < KIOAPIC.num_ovrds; i++)
        KIOAPIC.ovrds[i] = *(ioapic_so_t*) get_madt_item(madt, MADT_INT_SO_OVRD, i);

    KSMP.num_cpus = get_madt_count(madt, MADT_LAPIC);
    KCPUS = halloc(sizeof(cpu_data_t) * KSMP.num_cpus, KHEAP);
    memset(KCPUS, 0, sizeof(cpu_data_t) * KSMP.num_cpus);
}

size_t get_madt_count(madt_t *table, uint8_t search_item) {
    size_t count = 0;
    madt_item_t *item;
    while (true) {
        item = get_madt_item(table, search_item, count);
        if (!item) break;
        count++;
    }
    return count;
}

madt_item_t *get_madt_item(madt_t *table, uint8_t search_item, uint8_t count) {
    uint8_t counter = 0;
    madt_item_t *item = (madt_item_t*) (table + 1);
    while ((uintptr_t) item < ((uintptr_t) table) + table->header.length) {
        if (item->type == search_item && count == counter++)
            return item;
        item = (madt_item_t*) ((uintptr_t) item + item->length);
    }
    return NULL;
}