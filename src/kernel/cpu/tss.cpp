#include <mem/heap.hpp>
#include <cpu/gdt.hpp>
#include <cpu/tss.hpp>
#include <util/logger.h>
#include <acpi/madt.hpp>
using namespace cpu;

TSS::TSS(GDT& gdt) : gdt{gdt} {
    uintptr_t tss = reinterpret_cast<uintptr_t>(new struct tss());
    uint64_t gdt_entry = sizeof(struct tss) |
                         (tss & 0xFFFF) << 16 |
                         ((tss >> 16) & 0xFF) << 32 |
                         0b10001001UL << 40 |
                         ((tss >> 24) & 0xFF) << 56;
    uint16_t seg = gdt.entries * 8;

    gdt.set_entry(gdt.entries, { .raw = gdt_entry });
    gdt.set_entry(gdt.entries, { .raw = (tss >> 32) & 0xFFFFFFFF });

    gdt.load();
    asm volatile ("ltr %0" : : "r" (seg));
    log(Info, "TSS", "Initialized TSS");
}

void TSS::set_rsp0(uintptr_t rsp) const {
    uint16_t tr;
    asm volatile ("str %0" : "=rm" (tr));
    GDT::gdt_desc entry0 = gdt.get_entry(tr / 8);
    GDT::gdt_desc entry1 = gdt.get_entry(tr / 8 + 1);
    tss *tss = (struct tss*) (entry0.base0 |
                                (entry0.base1 << 16) |
                                (entry0.base2 << 24) |
                                ((entry1.raw & 0xFFFFFFFF) << 32));
    tss->rsp0 = rsp;
}
