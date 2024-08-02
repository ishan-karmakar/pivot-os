#include <cpu/gdt.hpp>
#include <algorithm>
#include <util/logger.hpp>
using namespace cpu;

frg::manual_box<GDT> cpu::kgdt;
constinit gdt::desc sgdt[3];

void gdt::early_init() {
    kgdt.initialize(sgdt);
    kgdt->set_entry(1, 0b10011011, 0b10); // Kernel CS
    kgdt->set_entry(2, 0b10010011, 0); // Kernel DS
    kgdt->load();
}

GDT::GDT(gdt::desc *gdt) : entries{1}, gdt{gdt} { gdt[0] = {}; }

GDT& GDT::operator=(GDT& old) {
    for (uint16_t i = 0; i < old.entries; i++)
        set_entry(i, old.get_entry(i));
    entries = std::max(entries, old.entries);
    return *this;
}

void GDT::set_entry(uint16_t idx, uint8_t access, uint8_t flags) {
    gdt::desc desc { { 0xFFFF, 0, 0, access, 0xF, flags, 0 } };

    set_entry(idx, desc);
}

void GDT::set_entry(uint16_t idx, gdt::desc desc) {
    gdt[idx] = desc;
    entries = std::max(static_cast<uint16_t>(idx + 1), entries);
}

gdt::desc GDT::get_entry(uint16_t idx) const { return gdt[idx]; }

void GDT::load() {
    gdtr.size = entries * sizeof(gdt::desc) - 1;
    asm volatile (
        "cli;"
        "lgdt %0;"
        "push $0x8;"
        "push $.L%=;"
        "lretq;"
        ".L%=:"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"
        "mov %%ax, %%ss;"
        : : "rm" (gdtr), "a" (0x10) : "memory"
    );
    log(Info, "GDT", "Loaded GDT");
};