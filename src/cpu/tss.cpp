#include <mem/heap.hpp>
#include <cpu/gdt.hpp>
#include <cpu/tss.hpp>
#include <cpu/smp.hpp>
#include <lib/logger.hpp>
using namespace tss;

void tss::init() {
    uintptr_t addr = reinterpret_cast<uintptr_t>(&smp::this_cpu()->tss);
    uint64_t gdt_entry = sizeof(tss) |
                         (addr & 0xFFFF) << 16 |
                         ((addr >> 16) & 0xFF) << 32 |
                         0b10001001UL << 40 |
                         ((addr >> 24) & 0xFF) << 56;

    uint16_t seg = gdt::num_entries * 8;

    gdt::set(gdt::num_entries, { .raw = gdt_entry });
    gdt::set(gdt::num_entries, { .raw = (addr >> 32) & 0xFFFFFFFF });

    gdt::load();
    asm volatile ("ltr %0" : : "r" (seg));
    logger::info("TSS[INIT]", "Initialized TSS");
}

void tss::set_rsp0(uintptr_t rsp) {
    smp::this_cpu()->tss.rsp0 = rsp;
}
