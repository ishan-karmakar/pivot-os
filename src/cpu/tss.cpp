#include <mem/heap.hpp>
#include <cpu/gdt.hpp>
#include <cpu/tss.hpp>
#include <lib/logger.hpp>
using namespace tss;

struct [[gnu::packed]] tss_t {
    uint32_t rsv0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t rsv1;
    uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    uint64_t rsv2;
    uint16_t rsv3;
    uint16_t iopb;
};

void tss::init() {
    uintptr_t tss = reinterpret_cast<uintptr_t>(new tss_t());
    uint64_t gdt_entry = sizeof(tss_t) |
                         (tss & 0xFFFF) << 16 |
                         ((tss >> 16) & 0xFF) << 32 |
                         0b10001001UL << 40 |
                         ((tss >> 24) & 0xFF) << 56;

    uint16_t seg = gdt::num_entries * 8;

    gdt::set(gdt::num_entries, { .raw = gdt_entry });
    gdt::set(gdt::num_entries, { .raw = (tss >> 32) & 0xFFFFFFFF });

    gdt::load();
    asm volatile ("ltr %0" : : "r" (seg));
    logger::info("TSS[INIT]", "Initialized TSS");
}

void tss::set_rsp0(uintptr_t rsp) {
    uint16_t tr;
    asm volatile ("str %0" : "=rm" (tr));
    gdt::desc entry0 = gdt::get(tr / 8);
    gdt::desc entry1 = gdt::get(tr / 8 + 1);
    tss_t *tss = (tss_t*) (entry0.base0 |
                                (entry0.base1 << 16) |
                                (entry0.base2 << 24) |
                                ((entry1.raw & 0xFFFFFFFF) << 32));
    tss->rsp0 = rsp;
}
