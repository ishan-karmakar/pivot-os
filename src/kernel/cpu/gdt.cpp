#include <cpu/gdt.hpp>
using namespace cpu;

GDT::GDT(gdt_desc * const gdt) : entries{1}, gdt{gdt} { gdt[0] = {}; }

GDT& GDT::operator=(GDT& old) {
    for (uint16_t i = 0; i < old.entries; i++)
        set_entry(i, old.get_entry(i));
    entries = std::max(entries, old.entries);
    return *this;
}

void GDT::set_entry(uint16_t idx, uint8_t access, uint8_t flags) {
    gdt_desc desc { { 0xFFFF, 0, 0, access, 0xF, flags, 0 } };

    set_entry(idx, desc);
}

void GDT::set_entry(uint16_t idx, gdt_desc desc) {
    gdt[idx] = desc;
    if (idx >= entries)
        entries = idx + 1;
}

GDT::gdt_desc GDT::get_entry(uint16_t idx) const { return gdt[idx]; }

void GDT::load() {
    gdtr.size = entries * sizeof(gdt_desc) - 1;
    asm volatile (
        "cli;"
        "lgdt %0;"
        "push $0x8;"
        "push $.L%=;"
        "lretq;"
        ".L%=:"
        "mov %1, %%ds;"
        "mov %1, %%es;"
        "mov %1, %%fs;"
        "mov %1, %%gs;"
        "mov %1, %%ss;"
        : : "rm" (gdtr), "r" (0x10) : "memory"
    );
    log(Info, "GDT", "Loaded GDT");
};